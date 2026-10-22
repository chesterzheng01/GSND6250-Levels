// GSND 6250 - GP3 Boss Weak Point

#include "GP3Boss.h"
#include "GP3HealthComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP3Boss::AGP3Boss()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// engine primitives so the greybox boss works without a blueprint; swap meshes and materials in BP_GP3_Boss
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	// the 100cm cube is centered on its pivot, so Z is the middle of each part:
	// skirt 0-220cm, torso 220-350cm, head 350-460cm, lining up with bullets fired from the ground/green, yellow and red tiers
	Skirt = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Skirt"));
	Skirt->SetupAttachment(Root);
	Skirt->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	Skirt->SetRelativeScale3D(FVector(2.6f, 2.6f, 2.2f));

	Torso = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Torso"));
	Torso->SetupAttachment(Root);
	Torso->SetRelativeLocation(FVector(0.0f, 0.0f, 285.0f));
	Torso->SetRelativeScale3D(FVector(1.8f, 1.8f, 1.3f));

	Head = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Head"));
	Head->SetupAttachment(Root);
	Head->SetRelativeLocation(FVector(0.0f, 0.0f, 405.0f));
	Head->SetRelativeScale3D(FVector(1.4f, 1.4f, 1.1f));

	if (CubeMesh.Succeeded())
	{
		Skirt->SetStaticMesh(CubeMesh.Object);
		Torso->SetStaticMesh(CubeMesh.Object);
		Head->SetStaticMesh(CubeMesh.Object);
	}

	Eye = CreateDefaultSubobject<USceneComponent>(TEXT("Eye"));
	Eye->SetupAttachment(Root);
	Eye->SetRelativeLocation(FVector(75.0f, 0.0f, 405.0f));

	EyeGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("EyeGlow"));
	EyeGlow->SetupAttachment(Eye);
	EyeGlow->SetLightColor(FLinearColor::Red);
	EyeGlow->SetIntensity(0.0f);
	EyeGlow->SetAttenuationRadius(800.0f);
	EyeGlow->SetCastShadows(false);

	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetupAttachment(Root);
	BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeamMesh->SetCastShadow(false);
	BeamMesh->SetHiddenInGame(true);

	// the beam is placed in world space when it fires, so it shouldn't follow the boss while turning
	BeamMesh->SetUsingAbsoluteLocation(true);
	BeamMesh->SetUsingAbsoluteRotation(true);
	BeamMesh->SetUsingAbsoluteScale(true);

	if (CylinderMesh.Succeeded())
	{
		BeamMesh->SetStaticMesh(CylinderMesh.Object);
	}

	Health = CreateDefaultSubobject<UGP3HealthComponent>(TEXT("Health"));
}

void AGP3Boss::BeginPlay()
{
	Super::BeginPlay();

	Health->OnDied.AddDynamic(this, &AGP3Boss::HandleDeath);
}

void AGP3Boss::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APawn* Player = GetLivingPlayer();
	if (!Player)
	{
		BeamState = EGP3BeamState::Idle;
		StateTime = 0.0f;
		EyeGlow->SetIntensity(0.0f);
		return;
	}

	const FVector Target = Player->GetActorLocation();

	switch (BeamState)
	{
	case EGP3BeamState::Idle:
		{
			FaceTarget(Target, DeltaSeconds);

			// only start charging once roughly facing the player, so the beam always leaves the front of the head
			const FVector ToTarget = (Target - GetActorLocation()).GetSafeNormal2D();
			const bool bFacing = FVector::DotProduct(GetActorForwardVector(), ToTarget) > 0.94f;

			if (bFacing && CanTarget(Player))
			{
				BeamState = EGP3BeamState::Charging;
				StateTime = 0.0f;
				AimPoint = Target;
				OnBeamCharge();
			}
		}
		break;

	case EGP3BeamState::Charging:
		StateTime += DeltaSeconds;

		if (StateTime < ChargeTime - AimLockTime)
		{
			AimPoint = Target;
			FaceTarget(Target, DeltaSeconds);
		}

		EyeGlow->SetIntensity(EyeGlowIntensity * FMath::Clamp(StateTime / FMath::Max(ChargeTime, KINDA_SMALL_NUMBER), 0.0f, 1.0f));

		if (StateTime >= ChargeTime)
		{
			FireBeam(Player);
			EyeGlow->SetIntensity(0.0f);
			BeamState = EGP3BeamState::Cooldown;
			StateTime = 0.0f;
		}
		break;

	case EGP3BeamState::Cooldown:
		FaceTarget(Target, DeltaSeconds);
		StateTime += DeltaSeconds;

		if (StateTime >= CooldownTime)
		{
			BeamState = EGP3BeamState::Idle;
			StateTime = 0.0f;
		}
		break;
	}
}

APawn* AGP3Boss::GetLivingPlayer() const
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player)
	{
		return nullptr;
	}

	const UGP3HealthComponent* PlayerHealth = Player->FindComponentByClass<UGP3HealthComponent>();
	return (PlayerHealth && PlayerHealth->IsDead()) ? nullptr : Player;
}

bool AGP3Boss::CanTarget(const APawn* Player) const
{
	const FVector PlayerLocation = Player->GetActorLocation();
	return FVector::Dist2D(PlayerLocation, GetActorLocation()) > DeadZoneRadius
		&& FVector::Dist(PlayerLocation, Eye->GetComponentLocation()) <= BeamRange;
}

void AGP3Boss::FaceTarget(const FVector& Target, float DeltaSeconds)
{
	const FVector ToTarget = Target - GetActorLocation();
	if (ToTarget.IsNearlyZero())
	{
		return;
	}

	const float NewYaw = FMath::FixedTurn(GetActorRotation().Yaw, ToTarget.Rotation().Yaw, TurnRate * DeltaSeconds);
	SetActorRotation(FRotator(0.0f, NewYaw, 0.0f));
}

void AGP3Boss::FireBeam(APawn* Player)
{
	const FVector Start = Eye->GetComponentLocation();
	const FVector Direction = (AimPoint - Start).GetSafeNormal();
	FVector End = Start + Direction * BeamRange;

	// find where cover stops the beam. The player is ignored here and tested against the capsule below,
	// so the result doesn't depend on how the character's collision channels are set up
	FCollisionQueryParams Params(SCENE_QUERY_STAT(GP3BossBeam), false, this);
	Params.AddIgnoredActor(Player);

	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		End = Hit.ImpactPoint;
	}

	bool bHitPlayer = false;

	if (CanTarget(Player))
	{
		float CapsuleRadius = 0.0f;
		float CapsuleHalfHeight = 0.0f;
		Player->GetSimpleCollisionCylinder(CapsuleRadius, CapsuleHalfHeight);

		const FVector Center = Player->GetActorLocation();
		const FVector AxisOffset(0.0f, 0.0f, FMath::Max(CapsuleHalfHeight - CapsuleRadius, 0.0f));

		FVector OnBeam;
		FVector OnCapsule;
		FMath::SegmentDistToSegmentSafe(Start, End, Center - AxisOffset, Center + AxisOffset, OnBeam, OnCapsule);

		if (FVector::Dist(OnBeam, OnCapsule) <= CapsuleRadius + BeamRadius)
		{
			bHitPlayer = true;
			End = OnBeam;

			// damage falls off with distance, so staying far away is safer but too far to shoot back
			const float Damage = FMath::GetMappedRangeValueClamped(
				FVector2D(NearDistance, FarDistance),
				FVector2D(NearDamage, FarDamage),
				FVector::Dist(Start, OnBeam));

			UGameplayStatics::ApplyDamage(Player, Damage, GetInstigatorController(), this, nullptr);
		}
	}

	// stretch the 100cm tall, 100cm wide cylinder from the eye to the end point
	const FVector BeamVector = End - Start;
	BeamMesh->SetWorldLocationAndRotation(Start + BeamVector * 0.5f, FRotationMatrix::MakeFromZ(BeamVector.GetSafeNormal()).Rotator());
	BeamMesh->SetWorldScale3D(FVector(BeamRadius * 2.0f / 100.0f, BeamRadius * 2.0f / 100.0f, BeamVector.Size() / 100.0f));
	BeamMesh->SetHiddenInGame(false);
	GetWorldTimerManager().SetTimer(BeamHideTimer, this, &AGP3Boss::HideBeam, FMath::Max(BeamVisibleTime, 0.01f), false);

	OnBeamFired(Start, End, bHitPlayer);
}

void AGP3Boss::HideBeam()
{
	BeamMesh->SetHiddenInGame(true);
}

float AGP3Boss::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Health->IsDead())
	{
		return 0.0f;
	}

	EGP3BossPart Part = EGP3BossPart::None;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		Part = GetPartFromComponent(static_cast<const FPointDamageEvent&>(DamageEvent).HitInfo.GetComponent());
	}

	float Multiplier = 0.0f;
	switch (Part)
	{
	case EGP3BossPart::Skirt:	Multiplier = SkirtDamageMultiplier; break;
	case EGP3BossPart::Torso:	Multiplier = TorsoDamageMultiplier; break;
	case EGP3BossPart::Head:	Multiplier = HeadDamageMultiplier; break;
	default: break;
	}

	const float Incoming = Super::TakeDamage(DamageAmount * Multiplier, DamageEvent, EventInstigator, DamageCauser);
	const float Dealt = FMath::Min(Incoming, Health->GetHealth());

	// notify before applying, because a killing blow destroys the boss inside ApplyDamage
	OnPartHit(Part, Dealt);
	Health->ApplyDamage(Incoming);

	return Dealt;
}

EGP3BossPart AGP3Boss::GetPartFromComponent(const UPrimitiveComponent* Component) const
{
	if (Component == Head)
	{
		return EGP3BossPart::Head;
	}
	if (Component == Torso)
	{
		return EGP3BossPart::Torso;
	}
	if (Component == Skirt)
	{
		return EGP3BossPart::Skirt;
	}
	return EGP3BossPart::None;
}

void AGP3Boss::HandleDeath()
{
	OnDefeated();
	GetWorldTimerManager().ClearTimer(BeamHideTimer);
	Destroy();
}
