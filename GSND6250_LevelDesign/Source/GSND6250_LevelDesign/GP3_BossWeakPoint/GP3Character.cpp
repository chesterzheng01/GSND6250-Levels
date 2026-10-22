// GSND 6250 - GP3 Boss Weak Point

#include "GP3Character.h"
#include "GP3HealthComponent.h"
#include "GP3Projectile.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AGP3Character::AGP3Character()
{
	Health = CreateDefaultSubobject<UGP3HealthComponent>(TEXT("Health"));

	ProjectileClass = AGP3Projectile::StaticClass();
}

void AGP3Character::BeginPlay()
{
	Super::BeginPlay();

	Health->OnDied.AddDynamic(this, &AGP3Character::HandleDeath);
}

void AGP3Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AGP3Character::DoFire);
	}
}

void AGP3Character::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (FireMappingContext)
			{
				Subsystem->AddMappingContext(FireMappingContext, 1);
			}
		}
	}
}

void AGP3Character::DoFire()
{
	if (Health->IsDead() || !ProjectileClass)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < FireCooldown)
	{
		return;
	}
	LastFireTime = Now;

	// shots are always level: only the camera's yaw is used, so the tier you stand on decides what you hit
	const FRotator AimRotation(0.0f, (GetController() ? GetControlRotation() : GetActorRotation()).Yaw, 0.0f);
	SetActorRotation(AimRotation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLocation = GetActorLocation() + AimRotation.Vector() * MuzzleForwardOffset;
	GetWorld()->SpawnActor<AGP3Projectile>(ProjectileClass, SpawnLocation, AimRotation, SpawnParams);

	OnFired();
}

float AGP3Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	return Health->ApplyDamage(Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser));
}

void AGP3Character::HandleDeath()
{
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		DisableInput(PlayerController);
	}
	GetCharacterMovement()->DisableMovement();

	OnDeath();

	GetWorldTimerManager().SetTimer(RestartTimer, this, &AGP3Character::RestartLevel, FMath::Max(RestartDelay, 0.01f), false);
}

void AGP3Character::RestartLevel()
{
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this)));
}
