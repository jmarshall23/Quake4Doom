// BSE_EffectTemplate.cpp
//

#include "precompiled.h"

#include "BSE_Envelope.h"
#include "BSE_Particle.h"
#include "BSE.h"
#include "BSE_SpawnDomains.h"

void rvDeclEffect::Init()
{
	mMinDuration = 0.0;
	mMaxDuration = 0.0;
	mSize = 512.0;
	mFlags = 0;
	mPlayCount = 0;
	mLoopCount = 0;
	mCutOffDistance = 0.0;
	mSegmentTemplates.Clear();
}

bool rvDeclEffect::SetDefaultText()
{
	char generated[1024]; // [esp+4h] [ebp-404h]

	idStr::snPrintf(generated, sizeof(generated), "effect %s // IMPLICITLY GENERATED\n");
	SetText(generated);
	return false;
}

size_t rvDeclEffect::Size(void) const {
	return sizeof(rvDeclEffect);
}

int rvDeclEffect::GetTrailSegmentIndex(const idStr& name)
{
	return 0;
}

float rvDeclEffect::EvaluateCost(int activeParticles, int segment) const
{
	return 0;
}

void rvDeclEffect::FreeData()
{
	
}

const char* rvDeclEffect::DefaultDefinition() const
{
	return "{\n}\n";
}

void rvDeclEffect::SetMinDuration(float duration)
{
	if (this->mMinDuration < duration)
		this->mMinDuration = duration;
}

void rvDeclEffect::SetMaxDuration(float duration)
{
	if (this->mMaxDuration < duration)
		this->mMaxDuration = duration;
}

void rvDeclEffect::Finish() {
	
}

bool rvDeclEffect::Parse(const char* text, const int textLength) {

	return true;
}
