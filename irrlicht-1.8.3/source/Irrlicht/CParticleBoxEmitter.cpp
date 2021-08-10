// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleBoxEmitter.h"
#include "os.h"
#include "IAttributes.h"
#include "irrMath.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleBoxEmitter::CParticleBoxEmitter(
	const core::aabbox3df& box, const core::vector3df& direction,
	u32 minParticlesPerSecond, u32 maxParticlesPerSecond,
	video::SColor minStartColor, video::SColor maxStartColor,
	u32 lifeTimeMin, u32 lifeTimeMax, s32 maxAngleDegrees,
	const core::dimension2df& minStartSize, const core::dimension2df& maxStartSize)
 : Box(box), Direction(direction),
	MaxStartSize(maxStartSize), MinStartSize(minStartSize),
	MinParticlesPerSecond(minParticlesPerSecond),
	MaxParticlesPerSecond(maxParticlesPerSecond),
	MinStartColor(minStartColor), MaxStartColor(maxStartColor),
	MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
	Time(0), Emitted(0), MaxAngleDegrees(maxAngleDegrees),
	FaxMin(0), FayMin(0), FazMin(0), FaxMax(0), FayMax(0), FazMax(0), RandomAngle(true), TimeInterval(30),
	m_bRandomX(true), m_bRandomY(true), m_bRandomZ(true), bKeepRotateSametoDirection(false), nTextureOrigAngle(0)
{
	#ifdef _DEBUG
	setDebugName("CParticleBoxEmitter");
	#endif

	m_dGlobalTransparency = 0.0;
	bEnableRotation2D = false;
	StartAngle2D = core::dimension2df(0.f, 0.f);
	EndAngle2D = core::dimension2df(0.f, 0.f);
	bSwing2D = false;
	SwingCount2D = core::dimension2df(0.f, 0.f);

	MinGravitySpeed = MaxGravitySpeed = core::vector3df(0.0f, -0.01f, 0.0f);
	MinRotateSpeed = MaxRotateSpeed = core::vector3df(0.0f, 0.0f, 0.0f);

	bTRANSPARENT_ADD_COLOR = true;

	bEnableFlashing = false;
	MinFlashSpeed = 0;
	MaxFlashSpeed = 0;
	MinFlashing = 0;
	MaxFlashing = 0;
}

//! Prepares an array with new particles to emitt into the system
//! and returns how much new particles there are.
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#include <math.h>

s32 CParticleBoxEmitter::makerAnglePositiveIn360(s32 Angle)
{
	if (Angle < 0)
	{
		int n = abs(Angle) / 360;
		Angle += 360 * (n + 1);
	}
	else
	{
		Angle %= 360;
	}
	
	return Angle;
}

void CParticleBoxEmitter::_emitt(u32 now, u32 amount)
{
	SParticle p;
	const core::vector3df& extent = Box.getExtent();
	for (u32 i=0; i<amount; ++i)
	{
		p.nOrigAngle = nTextureOrigAngle;
		p.bKeepRotateSametoDirection = bKeepRotateSametoDirection;

		//flashing
		p.enableFlashing = bEnableFlashing;
		p.flashSpeed = MinFlashSpeed + os::Randomizer::frand() * (MaxFlashSpeed - MinFlashSpeed);
		p.nFlashingOpacity = MinFlashing + os::Randomizer::frand() * (MaxFlashing - MinFlashing);
		p.bFlashOpacityAdd = true;
		p.nCurFlashOpacity = 0;

		//render mode
		p.TRANSPARENT_ADD_COLOR = bTRANSPARENT_ADD_COLOR;

		//gravity for gravity effector
		p.gravitySpeed.X = MinGravitySpeed.X + os::Randomizer::frand() * (MaxGravitySpeed.X - MinGravitySpeed.X);
		p.gravitySpeed.Y = MinGravitySpeed.Y + os::Randomizer::frand() * (MaxGravitySpeed.Y - MinGravitySpeed.Y);
		p.gravitySpeed.Z = MinGravitySpeed.Z + os::Randomizer::frand() * (MaxGravitySpeed.Z - MinGravitySpeed.Z);

		//rotate for rotate effector
		p.rotateSpeed.X = MinRotateSpeed.X + os::Randomizer::frand() * (MaxRotateSpeed.X - MinRotateSpeed.X);
		p.rotateSpeed.Y = MinRotateSpeed.Y + os::Randomizer::frand() * (MaxRotateSpeed.Y - MinRotateSpeed.Y);
		p.rotateSpeed.Z = MinRotateSpeed.Z + os::Randomizer::frand() * (MaxRotateSpeed.Z - MinRotateSpeed.Z);

		//rotate 2d
		p.enbleRotate2D = bEnableRotation2D;
		p.angle2DStart = StartAngle2D.Width + os::Randomizer::frand() * (StartAngle2D.Height - StartAngle2D.Width);
		p.angle2DEnd = EndAngle2D.Width + os::Randomizer::frand() * (EndAngle2D.Height - EndAngle2D.Width);
		p.angle2D = p.angle2DStart;
		p.swing2D = bSwing2D;
		p.swingCount2D = u32(SwingCount2D.Width + os::Randomizer::frand() * (SwingCount2D.Height - SwingCount2D.Width));

		//pos
		if (m_bRandomX)
		{
			p.pos.X = Box.MinEdge.X + os::Randomizer::frand() * extent.X;
		}
		else
		{
			if (amount > 1)
			{
				p.pos.X = Box.MinEdge.X + extent.X * f32(i) / f32(amount - 1);
			}
			else
			{
				p.pos.X = Box.MinEdge.X;
			}
		}

		if (m_bRandomY)
		{
			p.pos.Y = Box.MinEdge.Y + os::Randomizer::frand() * extent.Y;
		}
		else
		{
			if (amount > 1)
			{
				p.pos.Y = Box.MinEdge.Y + extent.Y * f32(i) / f32(amount - 1);
			}
			else
			{
				p.pos.Y = Box.MinEdge.Y;
			}
		}

		if (m_bRandomZ)
		{
			p.pos.Z = Box.MinEdge.Z + os::Randomizer::frand() * extent.Z;
		}
		else
		{
			if (amount > 1)
			{
				p.pos.Z = Box.MinEdge.Z + extent.Z * f32(i) / f32(amount - 1);
			}
			else
			{
				p.pos.Z = Box.MinEdge.Z;
			}
		}

		p.startTime = now;
		p.vector = Direction;

		{
			core::vector3df tgt = Direction;

			if (RandomAngle)
			{
				tgt.rotateXYBy(os::Randomizer::frand() * (FazMax - FazMin) + FazMin);
				tgt.rotateYZBy(os::Randomizer::frand() * (FaxMax - FaxMin) + FaxMin);
				tgt.rotateXZBy(os::Randomizer::frand() * (FayMax - FayMin) + FayMin);
			}
			else
			{
				f32 fax = f32(i) / f32(amount);
				f32 fay = f32(i) / f32(amount);
				f32 faz = f32(i) / f32(amount);
				if (makerAnglePositiveIn360(FaxMax) == makerAnglePositiveIn360(FaxMin))
				{
					fax = f32(i) / f32(amount);
				}
				else if (amount > 1)
				{
					fax = f32(i) / f32(amount - 1);
				}

				if (makerAnglePositiveIn360(FayMax) == makerAnglePositiveIn360(FayMin))
				{
					fay = f32(i) / f32(amount);
				}
				else if (amount > 1)
				{
					fay = f32(i) / f32(amount - 1);
				}

				if (makerAnglePositiveIn360(FazMax) == makerAnglePositiveIn360(FazMin))
				{
					faz = f32(i) / f32(amount);
				}
				else if (amount > 1)
				{
					faz = f32(i) / f32(amount - 1);
				}

				tgt.rotateXYBy(faz * (FazMax - FazMin) + FazMin);
				tgt.rotateYZBy(fax * (FaxMax - FaxMin) + FaxMin);
				tgt.rotateXZBy(fay * (FayMax - FayMin) + FayMin);
			}

			p.vector = tgt;
		}

		p.endTime = now + MIN(MinLifeTime, MaxLifeTime);
		if (MaxLifeTime != MinLifeTime)
		{
			if (MaxLifeTime > MinLifeTime)
			{
				p.endTime += os::Randomizer::rand() % (MaxLifeTime - MinLifeTime);
			}
			else
			{
				p.endTime += os::Randomizer::rand() % (MinLifeTime - MaxLifeTime);
			}
		}

		f32 c1 = (f32)MinStartColor.getAlpha();
		f32 c2 = (f32)MaxStartColor.getAlpha();
		f32 c3 = c1 + os::Randomizer::frand() * (c2 - c1);
		f32 c4 = c1 + os::Randomizer::frand() * (c2 - c1);
		MinStartColor.setAlpha(u32(c3));
		MaxStartColor.setAlpha(u32(c4));

		if (MinStartColor==MaxStartColor)
			p.color=MinStartColor;
		else
			p.color = MinStartColor.getInterpolated(MaxStartColor, os::Randomizer::frand());

		p.startColor = p.color;

		//make opacity with bmp
		if (p.TRANSPARENT_ADD_COLOR)
		{
			p.color = p.startColor.getInterpolated(
				video::SColor(0, 0, 0, 0), f32(p.color.getAlpha()) / 255.0f);
			p.startColor = p.color;
		}
		else
		{
			p.color.setAlpha(c3);
			p.startColor = p.color;
		}

		p.nStartOpacity = int(p.startColor.getAlpha());
		p.StartFlashColor = p.startColor;
		if (p.nFlashingOpacity > p.nStartOpacity)
		{
			p.nFlashingOpacity = p.nStartOpacity;
		}
		if (p.flashSpeed > p.nFlashingOpacity)
		{
			p.flashSpeed = p.nFlashingOpacity;
		}

		p.startVector = p.vector;

		if (MinStartSize==MaxStartSize)
			p.startSize = MinStartSize;
		else
			p.startSize = MinStartSize.getInterpolated(MaxStartSize, os::Randomizer::frand());
		p.size = p.startSize;

		Particles.push_back(p);
	}
}

s32 CParticleBoxEmitter::emitt(u32 now, u32 timeSinceLastCall, SParticle*& outArray)
{
	Time += timeSinceLastCall;
	
	const u32 pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
	const f32 perSecond = pps ? ((f32)MinParticlesPerSecond + os::Randomizer::frand() * pps) : MinParticlesPerSecond;
	
	if (Time > TimeInterval)
	{
		Particles.set_used(0);
		u32 amount = (u32)perSecond;
		Time = 0;
		_emitt(now, amount);
		outArray = Particles.pointer();
		return Particles.size();
	}

	return 0;
}


//! Writes attributes of the object.
void CParticleBoxEmitter::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	core::vector3df b = Box.getExtent();
	b *= 0.5f;
	out->addVector3d("Box", b);
	out->addVector3d("Direction", Direction);
	out->addFloat("MinStartSizeWidth", MinStartSize.Width);
	out->addFloat("MinStartSizeHeight", MinStartSize.Height);
	out->addFloat("MaxStartSizeWidth", MaxStartSize.Width);
	out->addFloat("MaxStartSizeHeight", MaxStartSize.Height); 
	out->addInt("MinParticlesPerSecond", MinParticlesPerSecond);
	out->addInt("MaxParticlesPerSecond", MaxParticlesPerSecond);
	out->addColor("MinStartColor", MinStartColor);
	out->addColor("MaxStartColor", MaxStartColor);
	out->addInt("MinLifeTime", MinLifeTime);
	out->addInt("MaxLifeTime", MaxLifeTime);
	out->addInt("MaxAngleDegrees", MaxAngleDegrees);
}


//! Reads attributes of the object.
void CParticleBoxEmitter::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	// read data and correct input values here

	core::vector3df b = in->getAttributeAsVector3d("Box");

	if (b.X <= 0)
		b.X = 1.0f;
	if (b.Y <= 0)
		b.Y = 1.0f;
	if (b.Z <= 0)
		b.Z = 1.0f;

	Box.MinEdge.X = -b.X;
	Box.MinEdge.Y = -b.Y;
	Box.MinEdge.Z = -b.Z;
	Box.MaxEdge.X = b.X;
	Box.MaxEdge.Y = b.Y;
	Box.MaxEdge.Z = b.Z;

	Direction = in->getAttributeAsVector3d("Direction");
	if (Direction.getLength() == 0)
		Direction.set(0,0.01f,0);

	int idx = -1;
	idx = in->findAttribute("MinStartSizeWidth");
	if ( idx >= 0 )
		MinStartSize.Width = in->getAttributeAsFloat(idx);
	idx = in->findAttribute("MinStartSizeHeight");
	if ( idx >= 0 )
		MinStartSize.Height = in->getAttributeAsFloat(idx);
	idx = in->findAttribute("MaxStartSizeWidth");
	if ( idx >= 0 )
		MaxStartSize.Width = in->getAttributeAsFloat(idx);
	idx = in->findAttribute("MaxStartSizeHeight");
	if ( idx >= 0 )
		MaxStartSize.Height = in->getAttributeAsFloat(idx); 

	MinParticlesPerSecond = in->getAttributeAsInt("MinParticlesPerSecond");
	MaxParticlesPerSecond = in->getAttributeAsInt("MaxParticlesPerSecond");

	MinParticlesPerSecond = core::max_(1u, MinParticlesPerSecond);
	MaxParticlesPerSecond = core::max_(MaxParticlesPerSecond, 1u);
	MaxParticlesPerSecond = core::min_(MaxParticlesPerSecond, 200u);
	MinParticlesPerSecond = core::min_(MinParticlesPerSecond, MaxParticlesPerSecond);

	MinStartColor = in->getAttributeAsColor("MinStartColor");
	MaxStartColor = in->getAttributeAsColor("MaxStartColor");
	MinLifeTime = in->getAttributeAsInt("MinLifeTime");
	MaxLifeTime = in->getAttributeAsInt("MaxLifeTime");
	MaxAngleDegrees = in->getAttributeAsInt("MaxAngleDegrees");

	MinLifeTime = core::max_(0u, MinLifeTime);
	MaxLifeTime = core::max_(MaxLifeTime, MinLifeTime);
	MinLifeTime = core::min_(MinLifeTime, MaxLifeTime);

}


} // end namespace scene
} // end namespace irr

