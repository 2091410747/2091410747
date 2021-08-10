// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __S_PARTICLE_H_INCLUDED__
#define __S_PARTICLE_H_INCLUDED__

#include "vector3d.h"
#include "dimension2d.h"
#include "SColor.h"

namespace irr
{
namespace scene
{
	//! Struct for holding particle data
	struct SParticle
	{
		//! Position of the particle
		core::vector3df pos;

		//! Direction and speed of the particle
		core::vector3df vector;

		//! Start life time of the particle
		u32 startTime;

		//! End life time of the particle
		u32 endTime;

		//! Current color of the particle
		video::SColor color;

		//! Original color of the particle.
		/** That's the color of the particle it had when it was emitted. */
		video::SColor startColor;

		//! Original direction and speed of the particle.
		/** The direction and speed the particle had when it was emitted. */
		core::vector3df startVector;

		//! Scale of the particle.
		/** The current scale of the particle. */
		core::dimension2df size;

		//! Original scale of the particle.
		/** The scale of the particle when it was emitted. */
		core::dimension2df startSize;

		//!2D rotation
		bool enbleRotate2D;
		f32 angle2D;
		f32 angle2DStart;
		f32 angle2DEnd;
		bool swing2D;
		u32 swingCount2D;

		bool bKeepRotateSametoDirection;
		int nOrigAngle;

		//gravity
		core::vector3df gravitySpeed;

		//3d rotate
		core::vector3df rotateSpeed;

		//
		bool TRANSPARENT_ADD_COLOR;

		//flashing
		bool enableFlashing;
		s32 flashSpeed;
		s32 nFlashingOpacity;
		s32 nStartOpacity;
		video::SColor StartFlashColor;
		s32 nCurFlashOpacity;
		bool bFlashOpacityAdd;

		//2d position
		double x, y, w, h;

		//1x1Á£×ÓÊ¹ÓÃ
		double dAppearDur, dDisappearDur;
		double dxAppearDel, dyAppearDel, dzAppearDel;
		double dxDisappearDel, dyDisappearDel, dzDisappearDel;
	};


} // end namespace scene
} // end namespace irr

#endif

