// BSE.h
//

#pragma once

class rvRenderEffectLocal;

class rvBSEManagerLocal : public rvBSEManager
{
public:
	virtual	bool				Init(void);
	virtual	bool				Shutdown(void);

	virtual	bool				PlayEffect(class rvRenderEffectLocal* def, float time);
	virtual	bool				ServiceEffect(class rvRenderEffectLocal* def, float time);
	virtual	void				StopEffect(rvRenderEffectLocal* def);
	virtual	void				FreeEffect(rvRenderEffectLocal* def);
	virtual	float				EffectDuration(const rvRenderEffectLocal* def);

	virtual	bool				CheckDefForSound(const renderEffect_t* def);

	virtual	void				BeginLevelLoad(void);
	virtual	void				EndLevelLoad(void);

	virtual	void				StartFrame(void);
	virtual	void				EndFrame(void);
	virtual bool				Filtered(const char* name, effectCategory_t category);

	virtual void				UpdateRateTimes(void) ;
	virtual bool				CanPlayRateLimited(effectCategory_t category);
};
