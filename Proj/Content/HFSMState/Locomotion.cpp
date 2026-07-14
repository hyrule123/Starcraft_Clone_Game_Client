#include "Content/pch.h"
#include "Locomotion.h"



namespace engine
{
	Locomotion::Locomotion()
		: Super(Locomotion::kClassConcreteName)
	{}
	Locomotion::~Locomotion()
	{}
	void Locomotion::OnEnter(const AIContext& ai_context)
	{
		Super::OnEnter(ai_context);


	}
}