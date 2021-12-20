#ifndef _RENDERERS_BASE_
#define _RENDERERS_BASE_

class Renderer
{
public:
	virtual void Init();
	virtual void Destroy();
	
	virtual void RenderFrame();
};

#endif