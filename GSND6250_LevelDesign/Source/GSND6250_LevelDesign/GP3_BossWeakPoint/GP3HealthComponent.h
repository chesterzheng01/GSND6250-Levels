// GSND 6250 - GP3 Boss Weak Point

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP3HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGP3HealthChangedSignature, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGP3DiedSignature);

/**
 *  Simple hit-point pool shared by the GP3 player and boss.
 *  Owners route their TakeDamage into ApplyDamage so each owner can decide how much damage actually lands.
 */
UCLASS(ClassGroup=(GP3), meta=(BlueprintSpawnableComponent))
class UGP3HealthComponent : public UActorComponent
{
	GENERATED_BODY()

protected:

	/** Hit points at the start of play */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta = (ClampMin = 1))
	float MaxHealth = 100.0f;

	/** Current hit points */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Health")
	float Health = 0.0f;

public:

	/** Called every time health changes. Bind a HUD widget to this */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FGP3HealthChangedSignature OnHealthChanged;

	/** Called once when health reaches zero */
	UPROPERTY(BlueprintAssignable, Category="Health")
	FGP3DiedSignature OnDied;

	/** Removes health. Returns the amount actually removed */
	UFUNCTION(BlueprintCallable, Category="Health")
	float ApplyDamage(float Amount);

	UFUNCTION(BlueprintPure, Category="Health")
	bool IsDead() const { return Health <= 0.0f; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category="Health")
	float GetMaxHealth() const { return MaxHealth; }

protected:

	virtual void BeginPlay() override;
};
