// GSND 6250 - GP3 Boss Weak Point

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP3Projectile.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 *  Short-range bullet fired by the GP3 player.
 *  Flies straight and level, and deals point damage to whatever it hits so the boss can tell which body part was struck.
 */
UCLASS()
class AGP3Projectile : public AActor
{
	GENERATED_BODY()

	/** Collision sphere, also the root */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* Collision;

	/** Visual only, no collision */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* Movement;

protected:

	/** Base damage before the boss applies its body-part multiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta = (ClampMin = 0))
	float Damage = 10.0f;

	/** Distance in cm the bullet travels before it disappears */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Projectile", meta = (ClampMin = 100, Units = "cm"))
	float Range = 1500.0f;

public:

	AGP3Projectile();

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
