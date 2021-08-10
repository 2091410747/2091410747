// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CParticleFadeOutAffector.h"
#include "IAttributes.h"
#include "os.h"

namespace irr
{
namespace scene
{

//! constructor
CParticleFadeOutAffector::CParticleFadeOutAffector(
	const video::SColor& targetColor, u32 fadeOutTime)
	: IParticleFadeOutAffector(), TargetColor(targetColor),
	EnableEndColors(false)
{

	#ifdef _DEBUG
	setDebugName("CParticleFadeOutAffector");
	#endif

	FadeOutTime = fadeOutTime ? static_cast<f32>(fadeOutTime) : 1.0f;
	FadeInTime = 0.0f;
}


//! Affects an array of particles.
void CParticleFadeOutAffector::affect(u32 now, SParticle* particlearray, u32 count)
{
	doFlashing(now, particlearray, count);

	if (FadeInTime > 0.00001f)
	{
		affectWithFadeIn(now, particlearray, count);
		return;
	}

	if (!Enabled)
	{
		return;
	}

	f32 d;

	for (u32 i=0; i<count; ++i)
	{
		f32 ft = FadeOutTime;
		if (particlearray[i].endTime - particlearray[i].startTime < ft)
		{
			ft = static_cast<f32>(particlearray[i].endTime - particlearray[i].startTime);
		}

		if (particlearray[i].endTime - now < ft && ft > 0.0001)
		{
			d = (particlearray[i].endTime - now) / ft;	// FadeOutTime probably f32 to save casts here (just guessing)
			if (particlearray[i].TRANSPARENT_ADD_COLOR)
			{
				particlearray[i].color = particlearray[i].startColor.getInterpolated(
					TargetColor, d);
			}
			else
			{
				particlearray[i].color.setAlpha(u32(f32(particlearray[i].startColor.getAlpha()) * d));
			}
		}
	}
}

void CParticleFadeOutAffector::affectWithFadeIn(u32 now, SParticle* particlearray, u32 count)
{
	if (!Enabled)
		return;
	f32 d;
	f32 dur;
	f32 fd1;
	f32 fd2;
	//TargetColor = video::SColor(255, 255, 255, 255);
	for (u32 i=0; i<count; ++i)
	{
		fd1 = FadeInTime;
		fd2 = FadeOutTime;
		dur = static_cast<f32>(particlearray[i].endTime - particlearray[i].startTime);
		if (fd1 + fd2 > dur)
		{
			fd1 = fd1 * dur / (fd1 + fd2);
			fd2 = dur - fd1;
		}

		if (now - particlearray[i].startTime < fd1)
		{
			d = (now - particlearray[i].startTime) / fd1;

			if (particlearray[i].TRANSPARENT_ADD_COLOR)
			{
				particlearray[i].color = particlearray[i].startColor.getInterpolated(
					TargetColor, d);
			}
			else
			{
				particlearray[i].color.setAlpha(u32(f32(particlearray[i].startColor.getAlpha()) * d));
			}
		}
		else if (particlearray[i].endTime - now < fd2 && fd2 > 0.0001)
		{
			d = (particlearray[i].endTime - now) / fd2;	// FadeOutTime probably f32 to save casts here (just guessing)
			//particlearray[i].color = particlearray[i].startColor.getInterpolated(
				//TargetColor, d);
			if (particlearray[i].TRANSPARENT_ADD_COLOR)
			{
				particlearray[i].color = particlearray[i].startColor.getInterpolated(
					TargetColor, d);
			}
			else
			{
				particlearray[i].color.setAlpha(u32(f32(particlearray[i].startColor.getAlpha()) * d));
			}
		}
	}
}

void CParticleFadeOutAffector::doFlashing(u32 now, SParticle* particlearray, u32 count)
{
	for (u32 i=0; i<count; ++i)
	{
		if (!particlearray[i].enableFlashing || particlearray[i].flashSpeed == 0 || particlearray[i].nFlashingOpacity == 0)
		{
			continue;
		}

		int ndur = particlearray[i].endTime - particlearray[i].startTime;
		if (ndur <= 0)
		{
			continue;
		}

		int nOP = particlearray[i].nStartOpacity;
		if (particlearray[i].bFlashOpacityAdd)
		{
			particlearray[i].nCurFlashOpacity += particlearray[i].flashSpeed;
			if (particlearray[i].nCurFlashOpacity >= particlearray[i].nFlashingOpacity)
			{
				particlearray[i].nCurFlashOpacity = particlearray[i].nFlashingOpacity;
				particlearray[i].bFlashOpacityAdd = false;
			}
			nOP = particlearray[i].nStartOpacity - particlearray[i].nCurFlashOpacity;
		}
		else
		{
			particlearray[i].nCurFlashOpacity -= particlearray[i].flashSpeed;
			if (particlearray[i].nCurFlashOpacity <= 0)
			{
				particlearray[i].nCurFlashOpacity = 0;
				particlearray[i].bFlashOpacityAdd = true;
			}
			nOP = particlearray[i].nStartOpacity - particlearray[i].nCurFlashOpacity;
		}

		if (nOP < 0)
		{
			nOP = 0;
		}
		if (nOP > 255)
		{
			nOP = 255;
		}

		if (particlearray[i].TRANSPARENT_ADD_COLOR)
		{
			if (particlearray[i].nStartOpacity > 0)
			{
				particlearray[i].color = particlearray[i].StartFlashColor.getInterpolated(
					TargetColor, f32(nOP) * f32(particlearray[i].nStartOpacity) / (255.0f * 255.0f));
			}
		}
		else
		{
			particlearray[i].color.setAlpha(u32(nOP));
		}

		particlearray[i].startColor = particlearray[i].color;
	}
}

void CParticleFadeOutAffector::doEndColors(u32 now, SParticle* particlearray, u32 count)
{
	return;

	if (!EnableEndColors)
	{
		return;
	}

	f32 dur;
	f32 dp;
	video::SColor colorCur;
	video::SColor colorCur1;
	video::SColor colorCur2;
	for (u32 i = 0; i < count; ++i)
	{
		if (now < particlearray[i].startTime || now > particlearray[i].endTime)
		{
			continue;
		}

		video::SColor colorMin(particlearray[i].startColor.getAlpha(), EndColorMin.getRed(), EndColorMin.getGreen(), EndColorMin.getBlue());
		dur = static_cast<f32>(particlearray[i].endTime - particlearray[i].startTime);
		dp = static_cast<f32>(now - particlearray[i].startTime) / dur;
		particlearray[i].color = particlearray[i].startColor.getInterpolated(
			colorMin, 1.0f - dp);
		continue;

		colorCur = EndColorMin.getInterpolated(EndColorMax, os::Randomizer::frand());
		colorCur2 = particlearray[i].startColor;
		colorCur2.setAlpha(255);
		colorCur1 = colorCur.getInterpolated(colorCur2, 1.0f - dp);
		
		particlearray[i].color = colorCur1.getInterpolated(
			video::SColor(0, 0, 0, 0), f32(particlearray[i].startColor.getAlpha()) / 255.0f);
		particlearray[i].color.setAlpha(255);
	}
}


//! Writes attributes of the object.
//! Implement this to expose the attributes of your scene node animator for
//! scripting languages, editors, debuggers or xml serialization purposes.
void CParticleFadeOutAffector::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addColor("TargetColor", TargetColor);
	out->addFloat("FadeOutTime", FadeOutTime);
	out->addFloat("FadeInTime", FadeInTime);
}

//! Reads attributes of the object.
//! Implement this to set the attributes of your scene node animator for
//! scripting languages, editors, debuggers or xml deserialization purposes.
//! \param startIndex: start index where to start reading attributes.
//! \return: returns last index of an attribute read by this affector
void CParticleFadeOutAffector::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	TargetColor = in->getAttributeAsColor("TargetColor");
	FadeOutTime = in->getAttributeAsFloat("FadeOutTime");
	FadeInTime = in->getAttributeAsFloat("FadeInTime");
}


} // end namespace scene
} // end namespace irr

