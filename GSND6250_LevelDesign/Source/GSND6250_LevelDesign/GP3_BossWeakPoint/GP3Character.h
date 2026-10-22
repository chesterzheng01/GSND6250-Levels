// GSND 6250 - GP3 Boss Weak Point

#pragma once

#include "CoreMinimal.h"
#include "GSND6250_LevelDesignCharacter.h"
#include "GP3Character.generated.h"

class UGP3HealthComponent;
class AGP3Projectile;
class UInputAction;
class UInputMappingContext;

/**
 *  Third person character for GP3.
 *  Adds a level, short-range gun and a health pool. Dying restarts the level.
 */
UCLASS(abstract)
class AGP3Character : public AGSND6250_LevelDesignCharacter
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UGP3HealthComponent* Health;

protected:

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* FireAction;

	/** Holds the fire binding. Added on top of the controller's default mapping contexts so the shared ones stay untouched */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputMappingContext* FireMappingContext;

	/** Bullet to spawn */
	UPROPERTY(EditAnywhere, Category="Shooting")
	TSubclassOf<AGP3Projectile> ProjectileClass;

	/** Minimum seconds between shots */
	UPROPERTY(EditAnywhere, Category="Shooting", meta = (ClampMin = 0, Units = "s"))
	float FireCooldown = 0.25f;

	/** How far in front of the capsule center the bullet spawns */
	UPROPERTY(EditAnywhere, Category="Shooting", meta = (ClampMin = 0, Units = "cm"))
	float MuzzleForwardOffset = 70.0f;

	/** Seconds between dying and the level restarting */
	UPROPERTY(EditAnywhere, Category="Health", meta = (ClampMin = 0, Units = "s"))
	float RestartDelay = 1.5f;

	/** World time of the last shot */
	float LastFireTime = -1000.0f;

	FTimerHandle RestartTimer;

public:

	AGP3Character();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	/** Fires one bullet straight ahead along the camera's yaw */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoFire();

protected:

	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Adds the fire mapping context once a player controller possesses us */
	virtual void NotifyControllerChanged() override;

	UFUNCTION()
	void HandleDeath();

	void RestartLevel();

	/** Called after each shot. Use for muzzle flash and sound */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooting")
	void OnFired();

	/** Called when health reaches zero, before the restart */
	UFUNCTION(BlueprintImplementableEvent, Category="Health")
	void OnDeath();
};
