// GSND 6250 - GP3 Boss Weak Point

#include "GP3HealthComponent.h"

void UGP3HealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

float UGP3HealthComponent::ApplyDamage(float Amount)
{
	if (Amount <= 0.0f || IsDead())
	{
		return 0.0f;
	}

	const float Removed = FMath::Min(Amount, Health);
	Health -= Removed;
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (IsDead())
	{
		OnDied.Broadcast();
	}

	return Removed;
}
