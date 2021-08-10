// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleSystemSceneNode.h"
#include "os.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"
#include "IVideoDriver.h"

#include "CParticleAnimatedMeshSceneNodeEmitter.h"
#include "CParticleBoxEmitter.h"
#include "CParticleCylinderEmitter.h"
#include "CParticleMeshEmitter.h"
#include "CParticlePointEmitter.h"
#include "CParticleRingEmitter.h"
#include "CParticleSphereEmitter.h"
#include "CParticleAttractionAffector.h"
#include "CParticleFadeOutAffector.h"
#include "CParticleGravityAffector.h"
#include "CParticleRotationAffector.h"
#include "CParticleScaleAffector.h"
#include "SViewFrustum.h"
#include <Windows.h>

#pragma warning(disable:4244 4305)

namespace irr
{
namespace scene
{

//! constructor
CParticleSystemSceneNode::CParticleSystemSceneNode(bool createDefaultEmitter,
	ISceneNode* parent, ISceneManager* mgr, s32 id,
	const core::vector3df& position, const core::vector3df& rotation,
	const core::vector3df& scale)
	: IParticleSystemSceneNode(parent, mgr, id, position, rotation, scale),
	Emitter(0), ParticleSize(core::dimension2d<f32>(5.0f, 5.0f)), LastEmitTime(0),
	MaxParticles(0xffff), Buffer(0), m_pBuffer(0), m_pIndices(0), m_bUseForTextTemplate(false), ParticlesAreGlobal(true)//ParticlesAreGlobal(true)要设置为false，开始饱满效果才对
{
	#ifdef _DEBUG
	setDebugName("CParticleSystemSceneNode");
	#endif

	memset(&ShakebyAudioData, 0, sizeof(RZShakebyAudioData));
	memset(&m_TextTemplateInfo, 0, sizeof(RZTextTemplateInfo));
	memset(&m_CurKeyPoint, 0, sizeof(RZTextTemplateKeyPoint));
	
	Buffer = new SMeshBuffer();
	if (createDefaultEmitter)
	{
		IParticleEmitter* e = createBoxEmitter();
		setEmitter(e);
		e->drop();
	}
}


//! destructor
CParticleSystemSceneNode::~CParticleSystemSceneNode()
{
	if (Emitter)
		Emitter->drop();
	if (Buffer)
		Buffer->drop();

	removeAllAffectors();
}

void CParticleSystemSceneNode::setDrawBuffer(void* pBuffer, void* pIndices)
{
	m_pBuffer = pBuffer;
	m_pIndices = pIndices;
}

//! Gets the particle emitter, which creates the particles.
IParticleEmitter* CParticleSystemSceneNode::getEmitter()
{
	return Emitter;
}


//! Sets the particle emitter, which creates the particles.
void CParticleSystemSceneNode::setEmitter(IParticleEmitter* emitter)
{
    if (emitter == Emitter)
        return;
	if (Emitter)
		Emitter->drop();

	Emitter = emitter;

	if (Emitter)
		Emitter->grab();
}


//! Adds new particle effector to the particle system.
void CParticleSystemSceneNode::addAffector(IParticleAffector* affector)
{
	affector->grab();
	AffectorList.push_back(affector);
}

//! Get a list of all particle affectors.
const core::list<IParticleAffector*>& CParticleSystemSceneNode::getAffectors() const
{
	return AffectorList;
}

//! Removes all particle affectors in the particle system.
void CParticleSystemSceneNode::removeAllAffectors()
{
	core::list<IParticleAffector*>::Iterator it = AffectorList.begin();
	while (it != AffectorList.end())
	{
		(*it)->drop();
		it = AffectorList.erase(it);
	}
}


//! Returns the material based on the zero based index i.
video::SMaterial& CParticleSystemSceneNode::getMaterial(u32 i)
{
	return Buffer->Material;
}


//! Returns amount of materials used by this scene node.
u32 CParticleSystemSceneNode::getMaterialCount() const
{
	return 1;
}


//! Creates a particle emitter for an animated mesh scene node
IParticleAnimatedMeshSceneNodeEmitter*
CParticleSystemSceneNode::createAnimatedMeshSceneNodeEmitter(
	scene::IAnimatedMeshSceneNode* node, bool useNormalDirection,
	const core::vector3df& direction, f32 normalDirectionModifier,
	s32 mbNumber, bool everyMeshVertex,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleAnimatedMeshSceneNodeEmitter( node,
			useNormalDirection, direction, normalDirectionModifier,
			mbNumber, everyMeshVertex,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a box particle emitter.
IParticleBoxEmitter* CParticleSystemSceneNode::createBoxEmitter(
	const core::aabbox3df& box, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees, const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleBoxEmitter(box, direction, minParticlesPerSecond,
		maxParticlesPerSecond, minStartColor, maxStartColor,
		lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a particle emitter for emitting from a cylinder
IParticleCylinderEmitter* CParticleSystemSceneNode::createCylinderEmitter(
	const core::vector3df& center, f32 radius,
	const core::vector3df& normal, f32 length,
	bool outlineOnly, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleCylinderEmitter( center, radius, normal, length,
			outlineOnly, direction,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a mesh particle emitter.
IParticleMeshEmitter* CParticleSystemSceneNode::createMeshEmitter(
	scene::IMesh* mesh, bool useNormalDirection,
	const core::vector3df& direction, f32 normalDirectionModifier,
	s32 mbNumber, bool everyMeshVertex,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize)
{
	return new CParticleMeshEmitter( mesh, useNormalDirection, direction,
			normalDirectionModifier, mbNumber, everyMeshVertex,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a point particle emitter.
IParticlePointEmitter* CParticleSystemSceneNode::createPointEmitter(
	const core::vector3df& direction, u32 minParticlesPerSecond,
	u32 maxParticlesPerSecond, const video::SColor& minStartColor,
	const video::SColor& maxStartColor, u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees, const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticlePointEmitter(direction, minParticlesPerSecond,
		maxParticlesPerSecond, minStartColor, maxStartColor,
		lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a ring particle emitter.
IParticleRingEmitter* CParticleSystemSceneNode::createRingEmitter(
	const core::vector3df& center, f32 radius, f32 ringThickness,
	const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize, const core::dimension2df& maxStartSize )
{
	return new CParticleRingEmitter( center, radius, ringThickness, direction,
		minParticlesPerSecond, maxParticlesPerSecond, minStartColor,
		maxStartColor, lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a sphere particle emitter.
IParticleSphereEmitter* CParticleSystemSceneNode::createSphereEmitter(
	const core::vector3df& center, f32 radius, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	const video::SColor& minStartColor, const video::SColor& maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax,
	s32 maxAngleDegrees, const core::dimension2df& minStartSize,
	const core::dimension2df& maxStartSize )
{
	return new CParticleSphereEmitter(center, radius, direction,
			minParticlesPerSecond, maxParticlesPerSecond,
			minStartColor, maxStartColor,
			lifeTimeMin, lifeTimeMax, maxAngleDegrees,
			minStartSize, maxStartSize );
}


//! Creates a point attraction affector. This affector modifies the positions of the
//! particles and attracts them to a specified point at a specified speed per second.
IParticleAttractionAffector* CParticleSystemSceneNode::createAttractionAffector(
	const core::vector3df& point, f32 speed, bool attract,
	bool affectX, bool affectY, bool affectZ )
{
	return new CParticleAttractionAffector( point, speed, attract, affectX, affectY, affectZ );
}

//! Creates a scale particle affector.
IParticleAffector* CParticleSystemSceneNode::createScaleParticleAffector(const core::dimension2df& scaleTo)
{
	return new CParticleScaleAffector(scaleTo);
}


//! Creates a fade out particle affector.
IParticleFadeOutAffector* CParticleSystemSceneNode::createFadeOutParticleAffector(
		const video::SColor& targetColor, u32 timeNeededToFadeOut)
{
	return new CParticleFadeOutAffector(targetColor, timeNeededToFadeOut);
}


//! Creates a gravity affector.
IParticleGravityAffector* CParticleSystemSceneNode::createGravityAffector(
		const core::vector3df& gravity, u32 timeForceLost)
{
	return new CParticleGravityAffector(gravity, timeForceLost);
}


//! Creates a rotation affector. This affector rotates the particles around a specified pivot
//! point.  The speed represents Degrees of rotation per second.
IParticleRotationAffector* CParticleSystemSceneNode::createRotationAffector(
	const core::vector3df& speed, const core::vector3df& pivotPoint )
{
	return new CParticleRotationAffector( speed, pivotPoint );
}

f32 MakerAnglePositive(f32 fAngle)
{
	f32 f = fAngle;
	if (f < 0.0)
	{
		int n = abs(f) / 360;
		f += 360.0 * (n + 1);
	}

	return f;
}

//! pre render event
void CParticleSystemSceneNode::OnRegisterSceneNode()
{
	//doParticleSystem(os::Timer::getTime());自己手动弄
	if (m_bUseForTextTemplate)
	{
		return;
	}

	if (Particles.size() != 0)
	{
		SceneManager->registerNodeForRendering(this);
		ISceneNode::OnRegisterSceneNode();
	}
}

void CParticleSystemSceneNode::setShakebyAudio(bool bEnable,
	bool bEnableZoom,
	bool bEnableOffset,
	bool bEnableTransparent,
	bool bRandomOffsetDirection,
	int nZoomMode,
	double dCurrentZoomPercent,
	double dCurrentXOffsetPercent,
	double dCurrentYOffsetPercent,
	double dCurrentTransparentPercent)
{
	ShakebyAudioData.bEnable = bEnable;
	ShakebyAudioData.bEnableOffset = bEnableOffset;
	ShakebyAudioData.bEnableTransparent = bEnableTransparent;
	ShakebyAudioData.bEnableZoom = bEnableZoom;
	ShakebyAudioData.bRandomOffsetDirection = bRandomOffsetDirection;
	ShakebyAudioData.nZoomMode = nZoomMode;
	ShakebyAudioData.dCurrentTransparentPercent = dCurrentTransparentPercent;
	ShakebyAudioData.dCurrentXOffsetPercent = dCurrentXOffsetPercent;
	ShakebyAudioData.dCurrentYOffsetPercent = dCurrentYOffsetPercent;
	ShakebyAudioData.dCurrentZoomPercent = dCurrentZoomPercent;

	if (!bEnableZoom && !bEnableOffset && !bEnableTransparent)
	{
		ShakebyAudioData.bEnable = false;
	}
}

//! render
void CParticleSystemSceneNode::render()
{
	if (!IsVisible)
	{
		return;
	}

	if (m_bUseForTextTemplate)
	{
		drawTextTemplateBuffer();
		//renderTextTemplate();
		return;
	}

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;


#if 0
	// calculate vectors for letting particles look to camera
	core::vector3df view(camera->getTarget() - camera->getAbsolutePosition());
	view.normalize();

	view *= -1.0f;

#else

	const core::matrix4 &m = camera->getViewFrustum()->getTransform( video::ETS_VIEW );

	const core::vector3df view ( -m[2], -m[6] , -m[10] );

#endif

	// reallocate arrays, if they are too small
	reallocateBuffers();

	/*#if 0
			core::vector3df horizontal = camera->getUpVector().crossProduct(view);
			horizontal.normalize();
			horizontal *= 0.5f * particle.size.Width;

			core::vector3df vertical = horizontal.crossProduct(view);
			vertical.normalize();
			vertical *= 0.5f * particle.size.Height;

		#else
			f32 f;

			f = 0.5f * particle.size.Width;
			const core::vector3df horizontal ( m[0] * f, m[4] * f, m[8] * f );

			f = -0.5f * particle.size.Height;
			const core::vector3df vertical ( m[1] * f, m[5] * f, m[9] * f );
		#endif*/
	// create particle vertex data
	s32 idx = 0;
	if (ShakebyAudioData.bEnable)
	{
		for (u32 i=0; i<Particles.size(); ++i)
		{
			const SParticle& particle = Particles[i];
			core::dimension2df size = particle.size;
			video::SColor color = particle.color;
			core::vector3df pos = particle.pos;
			if (ShakebyAudioData.bEnableOffset)
			{
				double dx = size.Width * ShakebyAudioData.dCurrentXOffsetPercent;
				double dy = -size.Height * ShakebyAudioData.dCurrentYOffsetPercent;
				if (ShakebyAudioData.bRandomOffsetDirection)
				{
					if (os::Randomizer::rand() % 2)
					{
						dx = -dx;
					}
					if (os::Randomizer::rand() % 2)
					{
						dy = -dy;
					}
				}
				pos.X += dx;
				pos.Y += dy;
			}
			if (ShakebyAudioData.bEnableZoom)
			{
				f32 fOldw = size.Width;
				f32 fOldh = size.Height;
				size.Width *= ShakebyAudioData.dCurrentZoomPercent;
				size.Height *= ShakebyAudioData.dCurrentZoomPercent;
				f32 dx = (size.Width - fOldw) / 2.0;
				f32 dy = -(size.Height - fOldh) / 2.0;
				int nMode = ShakebyAudioData.nZoomMode;
				if (9 == nMode)//random
				{
					nMode = os::Randomizer::rand() % 9;
				}
				if (nMode != 0)//!center
				{
					switch (nMode)
					{
					case 1: pos.X += dx; pos.Y += dy; break;
					case 2: pos.X -= dx; pos.Y += dy; break;
					case 3: pos.X += dx; pos.Y -= dy; break;
					case 4: pos.X -= dx; pos.Y -= dy; break;
					case 5: pos.X += dx; break;
					case 6: pos.X -= dx; break;
					case 7: pos.Y += dy; break;
					case 8: pos.Y -= dy; break;
					default: break;
					}
				}
			}
			if (ShakebyAudioData.bEnableTransparent)
			{
				u32 ua = color.getAlpha();
				ua = ua * (1.0 - ShakebyAudioData.dCurrentTransparentPercent);
				
				if (particle.TRANSPARENT_ADD_COLOR)
				{
					color = color.getInterpolated(
						video::SColor(0, 0, 0, 0), f32(ua) / 255.0f);
				}
				else
				{
					color.setAlpha(ua);
				}
			}

			f32 f;
			f = 0.5f * size.Width;
			const core::vector3df horizontal ( m[0] * f, m[4] * f, m[8] * f );
			f = -0.5f * size.Height;
			const core::vector3df vertical ( m[1] * f, m[5] * f, m[9] * f );

			Buffer->Vertices[0+idx].Pos = pos - horizontal - vertical;
			Buffer->Vertices[0+idx].Color = color;
			Buffer->Vertices[0+idx].Normal = view;

			Buffer->Vertices[1+idx].Pos = pos - horizontal + vertical;
			Buffer->Vertices[1+idx].Color = color;
			Buffer->Vertices[1+idx].Normal = view;

			Buffer->Vertices[2+idx].Pos = pos + horizontal + vertical;
			Buffer->Vertices[2+idx].Color = color;
			Buffer->Vertices[2+idx].Normal = view;

			Buffer->Vertices[3+idx].Pos = pos + horizontal - vertical;
			Buffer->Vertices[3+idx].Color = color;
			Buffer->Vertices[3+idx].Normal = view;

			idx +=4;
		}
	}
	else
	{
		//保持z为0时是实际大小，在textplamte中使用
		/*core::rect<s32> rvp = driver->getViewPort();
		double hmax = 40 * 2 * tan(camera->getFOV() / 2.0);
		double wmax = hmax * double(rvp.LowerRightCorner.X) / double(rvp.LowerRightCorner.Y);
		//f = f / rvp.LowerRightCorner.X * wmax;
		//f = f / rvp.LowerRightCorner.Y * hmax;*/
		for (u32 i=0; i<Particles.size(); ++i)
		{
			const SParticle& particle = Particles[i];

			f32 f;
			f = 0.5f * particle.size.Width;
			//f = f / rvp.LowerRightCorner.X * wmax;
			const core::vector3df horizontal ( m[0] * f, m[4] * f, m[8] * f );
			f = -0.5f * particle.size.Height;
			//f = f / rvp.LowerRightCorner.Y * hmax;
			const core::vector3df vertical ( m[1] * f, m[5] * f, m[9] * f );

			Buffer->Vertices[0+idx].Pos = particle.pos - horizontal - vertical;
			Buffer->Vertices[0+idx].Color = particle.color;
			Buffer->Vertices[0+idx].Normal = view;

			Buffer->Vertices[1+idx].Pos = particle.pos - horizontal + vertical;
			Buffer->Vertices[1+idx].Color = particle.color;
			Buffer->Vertices[1+idx].Normal = view;

			Buffer->Vertices[2+idx].Pos = particle.pos + horizontal + vertical;
			Buffer->Vertices[2+idx].Color = particle.color;
			Buffer->Vertices[2+idx].Normal = view;

			Buffer->Vertices[3+idx].Pos = particle.pos + horizontal - vertical;
			Buffer->Vertices[3+idx].Color = particle.color;
			Buffer->Vertices[3+idx].Normal = view;

			idx +=4;
		}
	}
	
	idx = 0;
	for (u32 i=0; i<Particles.size(); ++i)
	{
		f32 fKeepToDirectionAngle = 0;
		f32 fDelAngle = 0;
		if (Particles[i].bKeepRotateSametoDirection)
		{
			fDelAngle = 270.0;
			f32 fx = Particles[i].vector.X * 10000.0;
			f32 fy = Particles[i].vector.Y * 10000.0;
			if (fabs(fx) > 0.00001 && fabs(fy) > 0.00001)
			{
				fKeepToDirectionAngle = atan(fy / fx) * 180.0 / 3.14159265359f;
				if (fy > 0.0 && fx < 0.0)
				{
					fKeepToDirectionAngle += 180.0;
				}
				if (fy < 0.0 && fx < 0.0)
				{
					fKeepToDirectionAngle += 180.0;
				}
			}
			else if (fabs(fx) > 0.00001)
			{
				if (fx > 0.0)
				{
					fKeepToDirectionAngle = 0.0;
				}
				else
				{
					fKeepToDirectionAngle = 180.0;
				}
			}
			else if (fabs(fy) > 0.00001)
			{
				if (fy > 0.0)
				{
					fKeepToDirectionAngle = 90.0;
				}
				else
				{
					fKeepToDirectionAngle = 270.0;
				}
			}
		}

		f32 fa = Particles[i].nOrigAngle;
		fa = MakerAnglePositive(fa);
		fa = MakerAnglePositive(fa + fKeepToDirectionAngle + fDelAngle);//y正向为0度
		if (fabs(fa) > 0.00001)
		{
			Buffer->Vertices[0+idx].Pos.rotateXYBy(fa, Particles[i].pos);
			Buffer->Vertices[1+idx].Pos.rotateXYBy(fa, Particles[i].pos);
			Buffer->Vertices[2+idx].Pos.rotateXYBy(fa, Particles[i].pos);
			Buffer->Vertices[3+idx].Pos.rotateXYBy(fa, Particles[i].pos);
		}
		idx += 4;
	}

	//rotate
	idx = 0;
	for (u32 i=0; i<Particles.size(); ++i)
	{
		if (Particles[i].enbleRotate2D)
		{
			Buffer->Vertices[0+idx].Pos.rotateXYBy(Particles[i].angle2D, Particles[i].pos);
			Buffer->Vertices[1+idx].Pos.rotateXYBy(Particles[i].angle2D, Particles[i].pos);
			Buffer->Vertices[2+idx].Pos.rotateXYBy(Particles[i].angle2D, Particles[i].pos);
			Buffer->Vertices[3+idx].Pos.rotateXYBy(Particles[i].angle2D, Particles[i].pos);
		}
		idx += 4;
	}
	
	//global transparency
	double dt = Emitter->getGlobalTransparency();
	if (dt > 0.0001)
	{
		idx = 0;
		for (u32 i=0; i<Particles.size(); ++i)
		{
			const SParticle& particle = Particles[i];
			video::SColor color = particle.color;
			u32 ua = color.getAlpha();
			ua = ua * (1.0 - dt);

			if (particle.TRANSPARENT_ADD_COLOR)
			{
				color = color.getInterpolated(
					video::SColor(0, 0, 0, 0), f32(ua) / 255.0f);
			}
			else
			{
				color.setAlpha(ua);
			}

			Buffer->Vertices[0+idx].Color = color;
			Buffer->Vertices[1+idx].Color = color;
			Buffer->Vertices[2+idx].Color = color;
			Buffer->Vertices[3+idx].Color = color;

			idx += 4;
		}
	}

	// render all
	core::matrix4 mat;
	if (!ParticlesAreGlobal)
		mat.setTranslation(AbsoluteTransformation.getTranslation());
	driver->setTransform(video::ETS_WORLD, mat);
	
	driver->setMaterial(Buffer->Material);
	
	driver->drawVertexPrimitiveList(Buffer->getVertices(), Particles.size()*4,
		Buffer->getIndices(), Particles.size()*2, video::EVT_STANDARD, EPT_TRIANGLES,Buffer->getIndexType());

	// for debug purposes only:
	if ( DebugDataVisible & scene::EDS_BBOX )
	{
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		video::SMaterial deb_m;
		deb_m.Lighting = false;
		driver->setMaterial(deb_m);
		driver->draw3DBox(Buffer->BoundingBox, video::SColor(0,255,255,255));
	}
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CParticleSystemSceneNode::getBoundingBox() const
{
	return Buffer->getBoundingBox();
}

void CParticleSystemSceneNode::setCurrentParticlesOffset(f32 fdx, f32 fdy)
{
	for (u32 i=0; i < Particles.size(); ++i)
	{
		Particles[i].pos.X += fdx * (40.0f + Particles[i].pos.Z) / 20.0f;
		Particles[i].pos.Y -= fdy * (40.0f + Particles[i].pos.Z) / 20.0f;
	}
}

u32 CParticleSystemSceneNode::getCurrentParticlesCount()
{
	return Particles.size();
}

u32 CParticleSystemSceneNode::getLastEmitTime()
{
	return LastEmitTime;
}

void CParticleSystemSceneNode::setUseForTextTemplate(bool bUseForTextTemplate)
{
	m_bUseForTextTemplate = bUseForTextTemplate;
	/*if (m_bUseForTextTemplate)
	{
		if (Particles.empty())
		{
			SParticle p;
			p.TRANSPARENT_ADD_COLOR = false;
			//Particles.push_back(p);
		}
	}*/
}

void CParticleSystemSceneNode::drawTextTemplateBuffer()
{
	if (NULL == m_pBuffer)
	{
		return;
	}

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	if (NULL == driver)
	{
		return;
	}

	// render all
	driver->clearZBuffer();
	core::matrix4 mat;
	if (!ParticlesAreGlobal)
		mat.setTranslation(AbsoluteTransformation.getTranslation());
	driver->setTransform(video::ETS_WORLD, mat);
	
	driver->setMaterial(Buffer->Material);
	
	u32 size = ((SMeshBuffer*)m_pBuffer)->Vertices.size();
	driver->drawVertexPrimitiveList(((SMeshBuffer*)m_pBuffer)->getVertices(), size,
		((core::array<u32>*)m_pIndices)->const_pointer(), size / 2, video::EVT_STANDARD, EPT_TRIANGLES, video::EIT_32BIT);
}

void CParticleSystemSceneNode::renderTextTemplate()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	const core::matrix4 &m = camera->getViewFrustum()->getTransform( video::ETS_VIEW );
	const core::vector3df view ( -m[2], -m[6] , -m[10] );

	// reallocate arrays, if they are too small
	reallocateBuffers();

	//保持z为0时是实际大小，在textplamte中使用
	core::rect<s32> rvp = driver->getViewPort();
	double hmax = 40 * 2 * tan(camera->getFOV() / 2.0);
	double wmax = hmax * double(rvp.LowerRightCorner.X) / double(rvp.LowerRightCorner.Y);
	double dxScale = double(rvp.LowerRightCorner.X) / m_TextTemplateInfo.dOrigFrameW;
	double dyScale = double(rvp.LowerRightCorner.Y) / m_TextTemplateInfo.dOrigFrameH;

	// create particle vertex data
	bool bDone = false;
	s32 idx = 0;
	if (ShakebyAudioData.bEnable)
	{
		for (u32 i=0; i<Particles.size(); ++i)
		{
			change2DPositionTo3D(Particles[i], dxScale, dyScale);
			const SParticle& particle = Particles[i];
			core::dimension2df size = particle.size;
			video::SColor color = particle.color;
			core::vector3df pos = particle.pos;
			if (ShakebyAudioData.bEnableOffset)
			{
				double dx = size.Width * ShakebyAudioData.dCurrentXOffsetPercent;
				double dy = -size.Height * ShakebyAudioData.dCurrentYOffsetPercent;
				if (ShakebyAudioData.bRandomOffsetDirection)
				{
					if (os::Randomizer::rand() % 2)
					{
						dx = -dx;
					}
					if (os::Randomizer::rand() % 2)
					{
						dy = -dy;
					}
				}
				pos.X += dx;
				pos.Y += dy;
			}
			if (ShakebyAudioData.bEnableZoom)
			{
				f32 fOldw = size.Width;
				f32 fOldh = size.Height;
				size.Width *= ShakebyAudioData.dCurrentZoomPercent;
				size.Height *= ShakebyAudioData.dCurrentZoomPercent;
				f32 dx = (size.Width - fOldw) / 2.0;
				f32 dy = -(size.Height - fOldh) / 2.0;
				int nMode = ShakebyAudioData.nZoomMode;
				if (9 == nMode)//random
				{
					nMode = os::Randomizer::rand() % 9;
				}
				if (nMode != 0)//!center
				{
					switch (nMode)
					{
					case 1: pos.X += dx; pos.Y += dy; break;
					case 2: pos.X -= dx; pos.Y += dy; break;
					case 3: pos.X += dx; pos.Y -= dy; break;
					case 4: pos.X -= dx; pos.Y -= dy; break;
					case 5: pos.X += dx; break;
					case 6: pos.X -= dx; break;
					case 7: pos.Y += dy; break;
					case 8: pos.Y -= dy; break;
					default: break;
					}
				}
			}
			if (ShakebyAudioData.bEnableTransparent)
			{
				u32 ua = color.getAlpha();
				ua = ua * (1.0 - ShakebyAudioData.dCurrentTransparentPercent);
				
				if (particle.TRANSPARENT_ADD_COLOR)
				{
					color = color.getInterpolated(
						video::SColor(0, 0, 0, 0), f32(ua) / 255.0f);
				}
				else
				{
					color.setAlpha(ua);
				}
			}

			bool bInTime = true;
			if (particle.startTime > LastEmitTime || particle.endTime < LastEmitTime)
			{
				bInTime = false;
			}

			if (!bDone)
			{
				bDone = bInTime;
			}

			f32 f = 0.0f;
			if (bInTime)
			{
				f = 0.5f * size.Width;
				f = f / rvp.LowerRightCorner.X * wmax;
			}
			const core::vector3df horizontal ( m[0] * f, m[4] * f, m[8] * f );
			f = 0.0f;
			if (bInTime)
			{
				f = -0.5f * size.Height;
				f = f / rvp.LowerRightCorner.Y * hmax;
			}
			const core::vector3df vertical ( m[1] * f, m[5] * f, m[9] * f );

			Buffer->Vertices[0+idx].Pos = pos - horizontal - vertical;
			Buffer->Vertices[0+idx].Color = color;
			//Buffer->Vertices[0+idx].Normal = view;

			Buffer->Vertices[1+idx].Pos = pos - horizontal + vertical;
			Buffer->Vertices[1+idx].Color = color;
			//Buffer->Vertices[1+idx].Normal = view;

			Buffer->Vertices[2+idx].Pos = pos + horizontal + vertical;
			Buffer->Vertices[2+idx].Color = color;
			//Buffer->Vertices[2+idx].Normal = view;

			Buffer->Vertices[3+idx].Pos = pos + horizontal - vertical;
			Buffer->Vertices[3+idx].Color = color;
			//Buffer->Vertices[3+idx].Normal = view;

			idx +=4;
		}
	}
	else
	{	
		for (u32 i=0; i<Particles.size(); ++i)
		{
			change2DPositionTo3D(Particles[i], dxScale, dyScale);
			const SParticle& particle = Particles[i];
			bool bInTime = true;
			if (particle.startTime > LastEmitTime || particle.endTime < LastEmitTime)
			{
				bInTime = false;
			}

			if (!bDone)
			{
				bDone = bInTime;
			}

			f32 f = 0.0f;

			if (bInTime)
			{
				f = 0.5f * particle.size.Width * dxScale;
				f = f / rvp.LowerRightCorner.X * wmax;
				f *= m_CurKeyPoint.dxScale;
			}

			const core::vector3df horizontal ( m[0] * f, m[4] * f, m[8] * f );

			f = 0.0f;
			if (bInTime)
			{
				f = -0.5f * particle.size.Height * dyScale;
				f = f / rvp.LowerRightCorner.Y * hmax;
				f *= m_CurKeyPoint.dyScale;
			}

			const core::vector3df vertical ( m[1] * f, m[5] * f, m[9] * f );

			Buffer->Vertices[0+idx].Pos = particle.pos - horizontal - vertical;
			Buffer->Vertices[0+idx].Color = particle.color;
			Buffer->Vertices[0+idx].Color.setAlpha(255 * (1.0 - m_CurKeyPoint.dTransparency[0]));
			//Buffer->Vertices[0+idx].Normal = view;

			Buffer->Vertices[1+idx].Pos = particle.pos - horizontal + vertical;
			Buffer->Vertices[1+idx].Color = particle.color;
			//Buffer->Vertices[1+idx].Normal = view;
			Buffer->Vertices[1+idx].Color.setAlpha(255 * (1.0 - m_CurKeyPoint.dTransparency[1]));

			Buffer->Vertices[2+idx].Pos = particle.pos + horizontal + vertical;
			Buffer->Vertices[2+idx].Color = particle.color;
			//Buffer->Vertices[2+idx].Normal = view;
			Buffer->Vertices[2+idx].Color.setAlpha(255 * (1.0 - m_CurKeyPoint.dTransparency[2]));

			Buffer->Vertices[3+idx].Pos = particle.pos + horizontal - vertical;
			Buffer->Vertices[3+idx].Color = particle.color;
			//Buffer->Vertices[3+idx].Normal = view;
			Buffer->Vertices[3+idx].Color.setAlpha(255 * (1.0 - m_CurKeyPoint.dTransparency[3]));

			idx +=4;
		}
	}

	if (!bDone)
	{
		return;
	}

	if (m_TextTemplateInfo.nMode > 0)
	{
		u32 i = 0;
		double dtw = m_TextTemplateInfo.wTex;
		double dth = m_TextTemplateInfo.hTex;
		//dtw = getMaterial(0).TextureLayer[0].Texture->getOriginalSize().Width;
		//dth = getMaterial(0).TextureLayer[0].Texture->getOriginalSize().Height;
		double dpx = dtw / m_TextTemplateInfo.w;
		double dpy = dth / m_TextTemplateInfo.h;
		double dx = 0.0;
		double dy = 0.0;
		u32 nw = u32(m_TextTemplateInfo.w);
		u32 nh = u32(m_TextTemplateInfo.h);
		
		for (i=0; i<Particles.size(); i++)
		{
			int nx = i % nw;
			int ny = i / nw;
			dx = double(nx) * dpx;
			dy = double(ny) * dpy;
			Buffer->Vertices[0+4 * i].TCoords.set(dx / dtw, dy / dth);
			Buffer->Vertices[1+4 * i].TCoords.set(dx / dtw, dy / dth + dpy / dth);
			Buffer->Vertices[2+4 * i].TCoords.set(dx / dtw + dpx / dtw, dy / dth + dpy / dth);
			Buffer->Vertices[3+4 * i].TCoords.set(dx / dtw + dpx / dtw, dy / dth);
		}
	}

	//全局旋转
	idx = 0;
	core::vector3df posRef(0, 0, 0);
	change2DPositionTo3D(posRef, dxScale, dyScale);
	for (u32 i=0; i<Particles.size(); ++i)
	{
		f32 fa = m_CurKeyPoint.dzGlobalAngle;
		if (fabs(fa) > 0.00001)
		{
			Buffer->Vertices[0+idx].Pos.rotateXYBy(fa, posRef);
			Buffer->Vertices[1+idx].Pos.rotateXYBy(fa, posRef);
			Buffer->Vertices[2+idx].Pos.rotateXYBy(fa, posRef);
			Buffer->Vertices[3+idx].Pos.rotateXYBy(fa, posRef);
		}

		fa = m_CurKeyPoint.dyGlobalAngle;
		if (fabs(fa) > 0.00001)
		{
			Buffer->Vertices[0+idx].Pos.rotateXZBy(fa, posRef);
			Buffer->Vertices[1+idx].Pos.rotateXZBy(fa, posRef);
			Buffer->Vertices[2+idx].Pos.rotateXZBy(fa, posRef);
			Buffer->Vertices[3+idx].Pos.rotateXZBy(fa, posRef);
		}

		fa = m_CurKeyPoint.dxGlobalAngle;
		if (fabs(fa) > 0.00001)
		{
			Buffer->Vertices[0+idx].Pos.rotateYZBy(fa, posRef);
			Buffer->Vertices[1+idx].Pos.rotateYZBy(fa, posRef);
			Buffer->Vertices[2+idx].Pos.rotateYZBy(fa, posRef);
			Buffer->Vertices[3+idx].Pos.rotateYZBy(fa, posRef);
		}

		idx += 4;
	}

	//自身旋转
	idx = 0;
	for (u32 i=0; i<Particles.size(); ++i)
	{
		f32 fa = m_CurKeyPoint.dzSingleAngle;
		if (fabs(fa) > 0.00001)
		{
			Buffer->Vertices[0+idx].Pos.rotateXYBy(fa, Particles[i].pos);
			Buffer->Vertices[1+idx].Pos.rotateXYBy(fa, Particles[i].pos);
			Buffer->Vertices[2+idx].Pos.rotateXYBy(fa, Particles[i].pos);
			Buffer->Vertices[3+idx].Pos.rotateXYBy(fa, Particles[i].pos);
		}

		fa = m_CurKeyPoint.dySingleAngle;
		if (fabs(fa) > 0.00001)
		{
			Buffer->Vertices[0+idx].Pos.rotateXZBy(fa, Particles[i].pos);
			Buffer->Vertices[1+idx].Pos.rotateXZBy(fa, Particles[i].pos);
			Buffer->Vertices[2+idx].Pos.rotateXZBy(fa, Particles[i].pos);
			Buffer->Vertices[3+idx].Pos.rotateXZBy(fa, Particles[i].pos);
		}

		fa = m_CurKeyPoint.dxSingleAngle;
		if (fabs(fa) > 0.00001)
		{
			Buffer->Vertices[0+idx].Pos.rotateYZBy(fa, Particles[i].pos);
			Buffer->Vertices[1+idx].Pos.rotateYZBy(fa, Particles[i].pos);
			Buffer->Vertices[2+idx].Pos.rotateYZBy(fa, Particles[i].pos);
			Buffer->Vertices[3+idx].Pos.rotateYZBy(fa, Particles[i].pos);
		}

		idx += 4;
	}
	
	// render all
	driver->clearZBuffer();
	core::matrix4 mat;
	if (!ParticlesAreGlobal)
		mat.setTranslation(AbsoluteTransformation.getTranslation());
	driver->setTransform(video::ETS_WORLD, mat);
	
	driver->setMaterial(Buffer->Material);

	driver->drawVertexPrimitiveList(Buffer->getVertices(), Particles.size() * 4,
		Indices32.const_pointer(), Particles.size() * 2, video::EVT_STANDARD, EPT_TRIANGLES, video::EIT_32BIT);

	//driver->drawVertexPrimitiveList(Buffer->getVertices(), Particles.size() * 4,
		//Buffer->getIndices(), Particles.size() * 2, video::EVT_STANDARD, EPT_TRIANGLES, Buffer->getIndexType());
}

void CParticleSystemSceneNode::getDel1(SParticle& P, double& dx, double& dy, double& dz)
{
	if (0 == m_TextTemplateInfo.nMode)
	{
		return;
	}

	double dp = 0.0;
	if (LastEmitTime >= P.startTime && LastEmitTime <= P.startTime + P.dAppearDur * 1000.0 && P.dAppearDur > 0.001)
	{
		dp = double(LastEmitTime - P.startTime) / (P.dAppearDur * 1000.0);
		dx = (1.0 - dp) * P.dxAppearDel;
		dy = (1.0 - dp) * P.dyAppearDel;
		dz = (1.0 - dp) * P.dzAppearDel;
	}
	else if (LastEmitTime >= P.endTime - P.dDisappearDur * 1000.0 && LastEmitTime <= P.endTime && P.dDisappearDur > 0.001)
	{
		dp = double(double(LastEmitTime) - double(P.endTime) + P.dDisappearDur * 1000.0) / (P.dDisappearDur * 1000.0);
		dx = dp * P.dxDisappearDel;
		dy = dp * P.dyDisappearDel;
		dz = dp * P.dzDisappearDel;
	}
}

void CParticleSystemSceneNode::change2DPositionTo3D(SParticle& P, double dxScale, double dyScale)
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();
	core::rect<s32> rvp = driver->getViewPort();
	double hmax = 40 * 2 * tan(camera->getFOV() / 2.0);
	double wmax = hmax * double(rvp.LowerRightCorner.X) / double(rvp.LowerRightCorner.Y);

	double dxDel1(0.0), dyDel1(0.0), dzDel1(0.0);//1x1
	getDel1(P, dxDel1, dyDel1, dzDel1);
	P.pos.X = -wmax / 2.0 + (0.5 * P.size.Width + P.x + m_CurKeyPoint.dxDirection + dxDel1) * dxScale / rvp.LowerRightCorner.X * wmax;
	P.pos.Y = -hmax / 2.0 + (0.5 * P.size.Height + (m_TextTemplateInfo.dOrigFrameH - P.y - P.size.Height - m_CurKeyPoint.dyDirection - dyDel1)) * dyScale / rvp.LowerRightCorner.Y * hmax;
	P.pos.Z = (m_CurKeyPoint.dzDirection + dzDel1) * dyScale / rvp.LowerRightCorner.Y * hmax;
}

void CParticleSystemSceneNode::change2DPositionTo3D(core::vector3df& posRef, double dxScale, double dyScale)
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();
	core::rect<s32> rvp = driver->getViewPort();
	double hmax = 40 * 2 * tan(camera->getFOV() / 2.0);
	double wmax = hmax * double(rvp.LowerRightCorner.X) / double(rvp.LowerRightCorner.Y);

	posRef.X = -wmax / 2.0 + m_CurKeyPoint.dxPointRef * dxScale / rvp.LowerRightCorner.X * wmax;
	posRef.Y = -hmax / 2.0 + (m_TextTemplateInfo.dOrigFrameH - m_CurKeyPoint.dyPointRef) * dyScale / rvp.LowerRightCorner.Y * hmax;
	posRef.Z = m_CurKeyPoint.dzPointRef * dyScale / rvp.LowerRightCorner.Y * hmax;
}

f32 CParticleSystemSceneNode::getAngleXY(SParticle& P)
{
	f32 fKeepToDirectionAngle = 0;
	f32 fDelAngle = 0;
	if (P.bKeepRotateSametoDirection)
	{
		fDelAngle = 270.0;
		f32 fx = P.vector.X * 10000.0;
		f32 fy = P.vector.Y * 10000.0;
		if (fabs(fx) > 0.00001 && fabs(fy) > 0.00001)
		{
			fKeepToDirectionAngle = atan(fy / fx) * 180.0 / 3.14159265359f;
			if (fy > 0.0 && fx < 0.0)
			{
				fKeepToDirectionAngle += 180.0;
			}
			if (fy < 0.0 && fx < 0.0)
			{
				fKeepToDirectionAngle += 180.0;
			}
		}
		else if (fabs(fx) > 0.00001)
		{
			if (fx > 0.0)
			{
				fKeepToDirectionAngle = 0.0;
			}
			else
			{
				fKeepToDirectionAngle = 180.0;
			}
		}
		else if (fabs(fy) > 0.00001)
		{
			if (fy > 0.0)
			{
				fKeepToDirectionAngle = 90.0;
			}
			else
			{
				fKeepToDirectionAngle = 270.0;
			}
		}
	}

	f32 fa = P.nOrigAngle;
	fa = MakerAnglePositive(fa);
	fa = MakerAnglePositive(fa + fKeepToDirectionAngle + fDelAngle);//y正向为0度
	return fa;
}

f32 CParticleSystemSceneNode::getAngleXZ(SParticle& P)
{
	f32 fKeepToDirectionAngle = 0;
	f32 fDelAngle = 0;
	if (P.bKeepRotateSametoDirection)
	{
		fDelAngle = 270.0;
		f32 fx = P.vector.X * 10000.0;
		f32 fy = P.vector.Z * 10000.0;
		if (fabs(fx) > 0.00001 && fabs(fy) > 0.00001)
		{
			fKeepToDirectionAngle = atan(fy / fx) * 180.0 / 3.14159265359f;
			if (fy > 0.0 && fx < 0.0)
			{
				fKeepToDirectionAngle += 180.0;
			}
			if (fy < 0.0 && fx < 0.0)
			{
				fKeepToDirectionAngle += 180.0;
			}
		}
		else if (fabs(fx) > 0.00001)
		{
			if (fx > 0.0)
			{
				fKeepToDirectionAngle = 0.0;
			}
			else
			{
				fKeepToDirectionAngle = 180.0;
			}
		}
		else if (fabs(fy) > 0.00001)
		{
			if (fy > 0.0)
			{
				fKeepToDirectionAngle = 90.0;
			}
			else
			{
				fKeepToDirectionAngle = 270.0;
			}
		}
	}

	f32 fa = P.nOrigAngle;
	fa = MakerAnglePositive(fa);
	fa = MakerAnglePositive(fa + fKeepToDirectionAngle + fDelAngle);//y正向为0度
	return fa;
}

f32 CParticleSystemSceneNode::getAngleYZ(SParticle& P)
{
	f32 fKeepToDirectionAngle = 0;
	f32 fDelAngle = 0;
	if (P.bKeepRotateSametoDirection)
	{
		fDelAngle = 270.0;
		f32 fx = P.vector.Y * 10000.0;
		f32 fy = P.vector.Z * 10000.0;
		if (fabs(fx) > 0.00001 && fabs(fy) > 0.00001)
		{
			fKeepToDirectionAngle = atan(fy / fx) * 180.0 / 3.14159265359f;
			if (fy > 0.0 && fx < 0.0)
			{
				fKeepToDirectionAngle += 180.0;
			}
			if (fy < 0.0 && fx < 0.0)
			{
				fKeepToDirectionAngle += 180.0;
			}
		}
		else if (fabs(fx) > 0.00001)
		{
			if (fx > 0.0)
			{
				fKeepToDirectionAngle = 0.0;
			}
			else
			{
				fKeepToDirectionAngle = 180.0;
			}
		}
		else if (fabs(fy) > 0.00001)
		{
			if (fy > 0.0)
			{
				fKeepToDirectionAngle = 90.0;
			}
			else
			{
				fKeepToDirectionAngle = 270.0;
			}
		}
	}

	f32 fa = P.nOrigAngle;
	fa = MakerAnglePositive(fa);
	fa = MakerAnglePositive(fa + fKeepToDirectionAngle + fDelAngle);//y正向为0度
	return fa;
}

void CParticleSystemSceneNode::doParticleSystem(u32 time, u32 lastEmitTime)
{
	if (m_bUseForTextTemplate)
	{
		LastEmitTime = time;
		return;
	}

	if (LastEmitTime==0)
	{
		LastEmitTime = time;
		return;
	}

	LastEmitTime = lastEmitTime;
	u32 now = time;
	u32 timediff = time - LastEmitTime;
	LastEmitTime = time;

	// run emitter
	if (Emitter /*&& IsVisible*/)
	{
		SParticle* array = 0;
		s32 newParticles = Emitter->emitt(now, timediff, array);

		if (newParticles && array)
		{
			s32 j=Particles.size();
			if (newParticles > maxParticlesCountVisible - j)
			{
				newParticles = maxParticlesCountVisible - j;
			}
			Particles.set_used(j+newParticles);
			for (s32 i=j; i<j+newParticles; ++i)
			{
				Particles[i]=array[i-j];
				AbsoluteTransformation.rotateVect(Particles[i].startVector);
				if (ParticlesAreGlobal)
					AbsoluteTransformation.transformVect(Particles[i].pos);
			}
		}
	}

	// run affectors
	core::list<IParticleAffector*>::Iterator ait = AffectorList.begin();
	for (; ait != AffectorList.end(); ++ait)
		(*ait)->affect(now, Particles.pointer(), Particles.size());

	if (ParticlesAreGlobal)
		Buffer->BoundingBox.reset(AbsoluteTransformation.getTranslation());
	else
		Buffer->BoundingBox.reset(core::vector3df(0,0,0));

	// animate all particles
	f32 scale = (f32)timediff;

	for (u32 i=0; i<Particles.size();)
	{
		// erase is pretty expensive!
		if (now > Particles[i].endTime || now < Particles[i].startTime)
		{
			// Particle order does not seem to matter.
			// So we can delete by switching with last particle and deleting that one.
			// This is a lot faster and speed is very important here as the erase otherwise
			// can cause noticable freezes.
			Particles[i] = Particles[Particles.size()-1];
			Particles.erase( Particles.size()-1 );
		}
		else
		{
			Particles[i].pos += (Particles[i].vector * scale);

			Buffer->BoundingBox.addInternalPoint(Particles[i].pos);
			++i;
		}
	}

	const f32 m = (ParticleSize.Width > ParticleSize.Height ? ParticleSize.Width : ParticleSize.Height) * 0.5f;
	Buffer->BoundingBox.MaxEdge.X += m;
	Buffer->BoundingBox.MaxEdge.Y += m;
	Buffer->BoundingBox.MaxEdge.Z += m;

	Buffer->BoundingBox.MinEdge.X -= m;
	Buffer->BoundingBox.MinEdge.Y -= m;
	Buffer->BoundingBox.MinEdge.Z -= m;

	if (ParticlesAreGlobal)
	{
		core::matrix4 absinv( AbsoluteTransformation, core::matrix4::EM4CONST_INVERSE );
		absinv.transformBoxEx(Buffer->BoundingBox);
	}
}


//! Sets if the particles should be global. If it is, the particles are affected by
//! the movement of the particle system scene node too, otherwise they completely
//! ignore it. Default is true.
void CParticleSystemSceneNode::setParticlesAreGlobal(bool global)
{
	ParticlesAreGlobal = global;
}

//! Remove all currently visible particles
void CParticleSystemSceneNode::clearParticles()
{
	Particles.set_used(0);
}

//! Sets the size of all particles.
void CParticleSystemSceneNode::setParticleSize(const core::dimension2d<f32> &size)
{
	os::Printer::log("setParticleSize is deprecated, use setMinStartSize/setMaxStartSize in emitter.", irr::ELL_WARNING);
	//A bit of a hack, but better here than in the particle code
	if (Emitter)
	{
		Emitter->setMinStartSize(size);
		Emitter->setMaxStartSize(size);
	}
	ParticleSize = size;
}


void CParticleSystemSceneNode::reallocateBuffers()
{
	if (Particles.size() * 4 > Buffer->getVertexCount() ||
			Particles.size() * 6 > Buffer->getIndexCount())
	{
		u32 oldSize = Buffer->getVertexCount();
		Buffer->Vertices.set_used(Particles.size() * 4);

		u32 i;

		// fill remaining vertices设置文理坐标，动态文理可以改这里
		
		if (0 == m_TextTemplateInfo.nMode)
		{
			for (i=oldSize; i<Buffer->Vertices.size(); i+=4)
			{
				Buffer->Vertices[0+i].TCoords.set(0.0f, 0.0f);
				Buffer->Vertices[1+i].TCoords.set(0.0f, 1.0f);
				Buffer->Vertices[2+i].TCoords.set(1.0f, 1.0f);
				Buffer->Vertices[3+i].TCoords.set(1.0f, 0.0f);
			}
		}

		// fill remaining indices
		if (m_bUseForTextTemplate)
		{
			u32 oldIdxSize = Indices32.size();
			u32 oldvertices = oldSize;
			Indices32.set_used(Particles.size() * 6);
			for (i=oldIdxSize; i<Indices32.size(); i+=6)
			{
				Indices32[0+i] = (u32)0+oldvertices;
				Indices32[1+i] = (u32)2+oldvertices;
				Indices32[2+i] = (u32)1+oldvertices;
				Indices32[3+i] = (u32)0+oldvertices;
				Indices32[4+i] = (u32)3+oldvertices;
				Indices32[5+i] = (u32)2+oldvertices;
				oldvertices += 4;
			}
		}
		else
		{
			u32 oldIdxSize = Buffer->getIndexCount();
			u32 oldvertices = oldSize;
			Buffer->Indices.set_used(Particles.size() * 6);
			for (i=oldIdxSize; i<Buffer->Indices.size(); i+=6)
			{
				Buffer->Indices[0+i] = (u32)0+oldvertices;
				Buffer->Indices[1+i] = (u32)2+oldvertices;
				Buffer->Indices[2+i] = (u32)1+oldvertices;
				Buffer->Indices[3+i] = (u32)0+oldvertices;
				Buffer->Indices[4+i] = (u32)3+oldvertices;
				Buffer->Indices[5+i] = (u32)2+oldvertices;
				oldvertices += 4;
			}
		}

		// fill remaining indices
		/*u32 oldIdxSize = Buffer->getIndexCount();
		u32 oldvertices = oldSize;
		Buffer->Indices.set_used(Particles.size() * 6);
		
		for (i=oldIdxSize; i<Buffer->Indices.size(); i+=6)
		{
			Buffer->Indices[0+i] = (u32)0+oldvertices;
			Buffer->Indices[1+i] = (u32)2+oldvertices;
			Buffer->Indices[2+i] = (u32)1+oldvertices;
			Buffer->Indices[3+i] = (u32)0+oldvertices;
			Buffer->Indices[4+i] = (u32)3+oldvertices;
			Buffer->Indices[5+i] = (u32)2+oldvertices;
			oldvertices += 4;
		}*/
	}
}


//! Writes attributes of the scene node.
void CParticleSystemSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	IParticleSystemSceneNode::serializeAttributes(out, options);

	out->addBool("GlobalParticles", ParticlesAreGlobal);
	out->addFloat("ParticleWidth", ParticleSize.Width);
	out->addFloat("ParticleHeight", ParticleSize.Height);

	// write emitter

	E_PARTICLE_EMITTER_TYPE type = EPET_COUNT;
	if (Emitter)
		type = Emitter->getType();

	out->addEnum("Emitter", (s32)type, ParticleEmitterTypeNames);

	if (Emitter)
		Emitter->serializeAttributes(out, options);

	// write affectors

	E_PARTICLE_AFFECTOR_TYPE atype = EPAT_NONE;

	for (core::list<IParticleAffector*>::ConstIterator it = AffectorList.begin();
		it != AffectorList.end(); ++it)
	{
		atype = (*it)->getType();

		out->addEnum("Affector", (s32)atype, ParticleAffectorTypeNames);

		(*it)->serializeAttributes(out);
	}

	// add empty affector to make it possible to add further affectors

	if (options && options->Flags & io::EARWF_FOR_EDITOR)
		out->addEnum("Affector", EPAT_NONE, ParticleAffectorTypeNames);
}


//! Reads attributes of the scene node.
void CParticleSystemSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	IParticleSystemSceneNode::deserializeAttributes(in, options);

	ParticlesAreGlobal = in->getAttributeAsBool("GlobalParticles");
	ParticleSize.Width = in->getAttributeAsFloat("ParticleWidth");
	ParticleSize.Height = in->getAttributeAsFloat("ParticleHeight");

	// read emitter

	int emitterIdx = in->findAttribute("Emitter");
	if (emitterIdx == -1)
		return;

	if (Emitter)
		Emitter->drop();
	Emitter = 0;

	E_PARTICLE_EMITTER_TYPE type = (E_PARTICLE_EMITTER_TYPE)
		in->getAttributeAsEnumeration("Emitter", ParticleEmitterTypeNames);

	switch(type)
	{
	case EPET_POINT:
		Emitter = createPointEmitter();
		break;
	case EPET_ANIMATED_MESH:
		Emitter = createAnimatedMeshSceneNodeEmitter(NULL); // we can't set the node - the user will have to do this
		break;
	case EPET_BOX:
		Emitter = createBoxEmitter();
		break;
	case EPET_CYLINDER:
		Emitter = createCylinderEmitter(core::vector3df(0,0,0), 10.f, core::vector3df(0,1,0), 10.f);	// (values here don't matter)
		break;
	case EPET_MESH:
		Emitter = createMeshEmitter(NULL);	// we can't set the mesh - the user will have to do this
		break;
	case EPET_RING:
		Emitter = createRingEmitter(core::vector3df(0,0,0), 10.f, 10.f);	// (values here don't matter)
		break;
	case EPET_SPHERE:
		Emitter = createSphereEmitter(core::vector3df(0,0,0), 10.f);	// (values here don't matter)
		break;
	default:
		break;
	}

	u32 idx = 0;

#if 0
	if (Emitter)
		idx = Emitter->deserializeAttributes(idx, in);

	++idx;
#else
	if (Emitter)
		Emitter->deserializeAttributes(in);
#endif

	// read affectors

	removeAllAffectors();
	u32 cnt = in->getAttributeCount();

	while(idx < cnt)
	{
		const char* name = in->getAttributeName(idx);

		if (!name || strcmp("Affector", name))
			return;

		E_PARTICLE_AFFECTOR_TYPE atype =
			(E_PARTICLE_AFFECTOR_TYPE)in->getAttributeAsEnumeration(idx, ParticleAffectorTypeNames);

		IParticleAffector* aff = 0;

		switch(atype)
		{
		case EPAT_ATTRACT:
			aff = createAttractionAffector(core::vector3df(0,0,0));
			break;
		case EPAT_FADE_OUT:
			aff = createFadeOutParticleAffector();
			break;
		case EPAT_GRAVITY:
			aff = createGravityAffector();
			break;
		case EPAT_ROTATE:
			aff = createRotationAffector();
			break;
		case EPAT_SCALE:
			aff = createScaleParticleAffector();
			break;
		case EPAT_NONE:
		default:
			break;
		}

		++idx;

		if (aff)
		{
#if 0
			idx = aff->deserializeAttributes(idx, in, options);
			++idx;
#else
			aff->deserializeAttributes(in, options);
#endif

			addAffector(aff);
			aff->drop();
		}
	}
}

void CParticleSystemSceneNode::updateTextTemplateCurKeyPoint(void* kp)
{
	memcpy(&m_CurKeyPoint, kp, sizeof(RZTextTemplateKeyPoint));
}

} // end namespace scene
} // end namespace irr


