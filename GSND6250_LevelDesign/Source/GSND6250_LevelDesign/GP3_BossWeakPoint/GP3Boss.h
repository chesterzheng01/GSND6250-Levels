// GSND 6250 - GP3 Boss Weak Point

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP3Boss.generated.h"

class UGP3HealthComponent;
class UStaticMeshComponent;
class UPointLightComponent;

/** Which body part a bullet struck. Each part sits at the height of one platform tier */
UENUM(BlueprintType)
enum class EGP3BossPart : uint8
{
	None,
	Skirt,	// green tier: heavily armored
	Torso,	// yellow tier: reduced damage
	Head	// red tier: weak point
};

UENUM(BlueprintType)
enum class EGP3BeamState : uint8
{
	Idle,
	Charging,
	Cooldown
};

/**
 *  Stationary boss for GP3.
 *  Turns in place to face the player and fires a telegraphed eye beam. Platforms block the beam,
 *  and the area right under the boss is a dead zone where neither side can hurt the other.
 */
UCLASS()
class AGP3Boss : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Root;

	/** Lowest body part, matches the green tier */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Skirt;

	/** Middle body part, matches the yellow tier */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Torso;

	/** Weak point, matches the red tier */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Head;

	/** Beam origin, on the front of the head */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USceneComponent* Eye;

	/** Glows while the beam charges so the player can react */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPointLightComponent* EyeGlow;

	/** Stretched cylinder shown for a moment when the beam fires. Visual only */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UGP3HealthComponent* Health;

protected:

	/** Damage multiplier for bullets hitting the skirt */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Weak Points", meta = (ClampMin = 0))
	float SkirtDamageMultiplier = 0.0f;

	/** Damage multiplier for bullets hitting the torso */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Weak Points", meta = (ClampMin = 0))
	float TorsoDamageMultiplier = 0.4f;

	/** Damage multiplier for bullets hitting the head */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Weak Points", meta = (ClampMin = 0))
	float HeadDamageMultiplier = 1.0f;

	/** Yaw speed when turning to face the player, in degrees per second */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam", meta = (ClampMin = 0))
	float TurnRate = 90.0f;

	/** Seconds the eye glows before the beam fires */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam", meta = (ClampMin = 0, Units = "s"))
	float ChargeTime = 1.0f;

	/** The aim stops following the player this many seconds before firing, so moving at the last moment dodges the beam */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam", meta = (ClampMin = 0, Units = "s"))
	float AimLockTime = 0.3f;

	/** Seconds between a shot and the next charge */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam", meta = (ClampMin = 0, Units = "s"))
	float CooldownTime = 3.0f;

	/** Players closer than this (horizontally) are under the boss and cannot be hit */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam", meta = (ClampMin = 0, Units = "cm"))
	float DeadZoneRadius = 300.0f;

	/** Maximum beam length */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam", meta = (ClampMin = 0, Units = "cm"))
	float BeamRange = 5000.0f;

	/** Half-thickness of the beam when testing whether it touches the player */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam", meta = (ClampMin = 0, Units = "cm"))
	float BeamRadius = 15.0f;

	/** Damage at NearDistance or closer */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam|Damage", meta = (ClampMin = 0))
	float NearDamage = 20.0f;

	/** Damage at FarDistance or farther */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam|Damage", meta = (ClampMin = 0))
	float FarDamage = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam|Damage", meta = (ClampMin = 0, Units = "cm"))
	float NearDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam|Damage", meta = (ClampMin = 0, Units = "cm"))
	float FarDistance = 3500.0f;

	/** Eye light brightness at the end of the charge */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam|Visuals", meta = (ClampMin = 0))
	float EyeGlowIntensity = 50000.0f;

	/** Seconds the beam mesh stays visible after firing */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Boss|Beam|Visuals", meta = (ClampMin = 0, Units = "s"))
	float BeamVisibleTime = 0.25f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Boss|Beam")
	EGP3BeamState BeamState = EGP3BeamState::Idle;

	/** Time spent in the current beam state */
	float StateTime = 0.0f;

	/** Where the beam will go, frozen during the aim lock */
	FVector AimPoint = FVector::ZeroVector;

	FTimerHandle BeamHideTimer;

public:

	AGP3Boss();

	virtual void Tick(float DeltaSeconds) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Which body part a component belongs to */
	UFUNCTION(BlueprintPure, Category="Boss")
	EGP3BossPart GetPartFromComponent(const UPrimitiveComponent* Component) const;

protected:

	virtual void BeginPlay() override;

	/** Returns the player pawn if it is alive */
	APawn* GetLivingPlayer() const;

	/** True if the player is outside the dead zone and within beam range */
	bool CanTarget(const APawn* Player) const;

	void FaceTarget(const FVector& Target, float DeltaSeconds);

	void FireBeam(APawn* Player);

	void HideBeam();

	UFUNCTION()
	void HandleDeath();

	/** Called when a bullet lands. Damage is zero on the armored skirt. Use for hit flashes and sounds */
	UFUNCTION(BlueprintImplementableEvent, Category="Boss")
	void OnPartHit(EGP3BossPart Part, float DamageDealt);

	/** Called when the beam starts charging */
	UFUNCTION(BlueprintImplementableEvent, Category="Boss")
	void OnBeamCharge();

	/** Called when the beam fires. bHitPlayer is false if cover or a dodge stopped it */
	UFUNCTION(BlueprintImplementableEvent, Category="Boss")
	void OnBeamFired(FVector Start, FVector End, bool bHitPlayer);

	/** Called just before the boss is removed from the level */
	UFUNCTION(BlueprintImplementableEvent, Category="Boss")
	void OnDefeated();
};
