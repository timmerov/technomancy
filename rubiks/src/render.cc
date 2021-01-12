/*
Copyright (C) 2012-2020 tim cotter. All rights reserved.
*/

/**
spinning rubik's cube.

the rubik's cube is a 3x3x3 stack of identical colored cubes.
we don't render the center cube.
cause it can never be seen.
**/

#include "render.h"

#include <aggiornamento/aggiornamento.h>
#include <aggiornamento/log.h>
#include <aggiornamento/opengl.h>
#include <common/png.h>

#include <GLES3/gl3.h>

// sigh...
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
//#include <glm/gtx/quaternion.hpp>

#include <iomanip>
#include <random>
#include <sstream>

#if !defined(XK_MISCELLANY)
#define XK_MISCELLANY 1
#endif
#include <X11/keysymdef.h>


namespace {
    auto g_vertex_source =R"shader_code(
        #version 310 es
        layout (location = 0) in vec3 vertex_pos_in;
        uniform mat4 model_mat;
        uniform mat4 proj_view_mat;
        void main() {
            vec4 world_pos = model_mat * vec4(vertex_pos_in, 1.0f);
            gl_Position = proj_view_mat * world_pos;
        }
    )shader_code";

    auto g_fragment_source = R"shader_code(
        #version 310 es
        out mediump vec4 color_out;
        uniform mediump vec3 color;
        void main() {
			color_out.rgb = color;
            color_out.a = 1.0f;
        }
    )shader_code";

	const float kFramesPerSecond = 60.0f;
	const float kSecondsPerRotation = 0.5f;
	const int kFramesPerRotation = (int)( kFramesPerSecond * kSecondsPerRotation );

	const int kNumFaceCubes = 9;
	const int kNumCubes = 26;
	const int kNumStates = 24;

    const GLfloat kA = 0.50f;  /// sub-cube size
    const GLfloat kB = 0.45f;  /// sticker size
    const GLfloat kC = 0.48f;  /// bevel
    const GLfloat g_cube_vertexes[] {
		+kC, +kC, +kC, /// 0+8*0
		-kC, +kC, +kC, /// 1+8*0
		+kB, +kB, +kA, /// 2+8*0
		-kB, +kB, +kA, /// 3+8*0
		+kB, -kB, +kA, /// 4+8*0
		-kB, -kB, +kA, /// 5+8*0
		+kC, -kC, +kC, /// 6+8*0
		-kC, -kC, +kC, /// 7+8*0

		-kC, +kC, -kC, /// 0+8*1
		+kC, +kC, -kC, /// 1+8*1
		-kB, +kB, -kA, /// 2+8*1
		+kB, +kB, -kA, /// 3+8*1
		-kB, -kB, -kA, /// 4+8*1
		+kB, -kB, -kA, /// 5+8*1
		-kC, -kC, -kC, /// 6+8*1
		+kC, -kC, -kC, /// 7+8*1

		+kC, +kC, +kC, /// 0+8*2
		+kC, -kC, +kC, /// 1+8*2
		+kA, +kB, +kB, /// 2+8*2
		+kA, -kB, +kB, /// 3+8*2
		+kA, +kB, -kB, /// 4+8*2
		+kA, -kB, -kB, /// 5+8*2
		+kC, +kC, -kC, /// 6+8*2
		+kC, -kC, -kC, /// 7+8*2

		-kC, +kC, -kC, /// 0+8*3
		-kC, -kC, -kC, /// 1+8*3
		-kA, +kB, -kB, /// 2+8*3
		-kA, -kB, -kB, /// 3+8*3
		-kA, +kB, +kB, /// 4+8*3
		-kA, -kB, +kB, /// 5+8*3
		-kC, +kC, +kC, /// 6+8*3
		-kC, -kC, +kC, /// 7+8*3

		-kC, +kC, +kC, /// 0+8*4
		+kC, +kC, +kC, /// 1+8*4
		-kB, +kA, +kB, /// 2+8*4
		+kB, +kA, +kB, /// 3+8*4
		-kB, +kA, -kB, /// 4+8*4
		+kB, +kA, -kB, /// 5+8*4
		-kC, +kC, -kC, /// 6+8*4
		+kC, +kC, -kC, /// 7+8*4

		-kC, -kC, -kC, /// 0+8*5
		+kC, -kC, -kC, /// 1+8*5
		-kB, -kA, -kB, /// 2+8*5
		+kB, -kA, -kB, /// 3+8*5
		-kB, -kA, +kB, /// 4+8*5
		+kB, -kA, +kB, /// 5+8*5
		-kC, -kC, +kC, /// 6+8*5
		+kC, -kC, +kC, /// 7+8*5
	};
	const GLfloat g_colors[] = {
		0.7f, 0.0f, 0.0f, /// red
		1.0f, 0.5f, 0.0f, /// orange
		0.0f, 0.0f, 0.8f, /// blue
		0.0f, 0.3f, 0.0f, /// green
		1.0f, 1.0f, 1.0f, /// white
		0.9f, 0.9f, 0.0f, /// yellow
	};
	const GLushort g_bevel_indexes[] = {
		8*0+0, 8*0+1, 8*0+2,
		8*0+0, 8*0+2, 8*0+4,
		8*0+0, 8*0+4, 8*0+6,
		8*0+1, 8*0+3, 8*0+2,
		8*0+1, 8*0+7, 8*0+3,
		8*0+3, 8*0+7, 8*0+5,
		8*0+4, 8*0+5, 8*0+6,
		8*0+5, 8*0+7, 8*0+6,

		8*1+0, 8*1+1, 8*1+2,
		8*1+0, 8*1+2, 8*1+4,
		8*1+0, 8*1+4, 8*1+6,
		8*1+1, 8*1+3, 8*1+2,
		8*1+1, 8*1+7, 8*1+3,
		8*1+3, 8*1+7, 8*1+5,
		8*1+4, 8*1+5, 8*1+6,
		8*1+5, 8*1+7, 8*1+6,

		8*2+0, 8*2+1, 8*2+2,
		8*2+0, 8*2+2, 8*2+4,
		8*2+0, 8*2+4, 8*2+6,
		8*2+1, 8*2+3, 8*2+2,
		8*2+1, 8*2+7, 8*2+3,
		8*2+3, 8*2+7, 8*2+5,
		8*2+4, 8*2+5, 8*2+6,
		8*2+5, 8*2+7, 8*2+6,

		8*3+0, 8*3+1, 8*3+2,
		8*3+0, 8*3+2, 8*3+4,
		8*3+0, 8*3+4, 8*3+6,
		8*3+1, 8*3+3, 8*3+2,
		8*3+1, 8*3+7, 8*3+3,
		8*3+3, 8*3+7, 8*3+5,
		8*3+4, 8*3+5, 8*3+6,
		8*3+5, 8*3+7, 8*3+6,

		8*4+0, 8*4+1, 8*4+2,
		8*4+0, 8*4+2, 8*4+4,
		8*4+0, 8*4+4, 8*4+6,
		8*4+1, 8*4+3, 8*4+2,
		8*4+1, 8*4+7, 8*4+3,
		8*4+3, 8*4+7, 8*4+5,
		8*4+4, 8*4+5, 8*4+6,
		8*4+5, 8*4+7, 8*4+6,

		8*5+0, 8*5+1, 8*5+2,
		8*5+0, 8*5+2, 8*5+4,
		8*5+0, 8*5+4, 8*5+6,
		8*5+1, 8*5+3, 8*5+2,
		8*5+1, 8*5+7, 8*5+3,
		8*5+3, 8*5+7, 8*5+5,
		8*5+4, 8*5+5, 8*5+6,
		8*5+5, 8*5+7, 8*5+6,
	};
	const GLushort g_face_indexes[] = {
		8*0+2, 8*0+3, 8*0+4,
		8*0+3, 8*0+5, 8*0+4,
		8*1+2, 8*1+3, 8*1+4,
		8*1+3, 8*1+5, 8*1+4,
		8*2+2, 8*2+3, 8*2+4,
		8*2+3, 8*2+5, 8*2+4,
		8*3+2, 8*3+3, 8*3+4,
		8*3+3, 8*3+5, 8*3+4,
		8*4+2, 8*4+3, 8*4+4,
		8*4+3, 8*4+5, 8*4+4,
		8*5+2, 8*5+3, 8*5+4,
		8*5+3, 8*5+5, 8*5+4,
	};
	const glm::vec3 g_xyz[kNumCubes] = {
		{+1.0f, +1.0f, +1.0f},  /// 0
		{+1.0f, +1.0f, +0.0f},  /// 1
		{+1.0f, +1.0f, -1.0f},  /// 2
		{+1.0f, +0.0f, +1.0f},  /// 3
		{+1.0f, +0.0f, +0.0f},  /// 4
		{+1.0f, +0.0f, -1.0f},  /// 5
		{+1.0f, -1.0f, +1.0f},  /// 6
		{+1.0f, -1.0f, +0.0f},  /// 7
		{+1.0f, -1.0f, -1.0f},  /// 8

		{+0.0f, +1.0f, +1.0f},  /// 9
		{+0.0f, +1.0f, +0.0f},  /// 10
		{+0.0f, +1.0f, -1.0f},  /// 11
		{+0.0f, +0.0f, +1.0f},  /// 12
		{+0.0f, +0.0f, -1.0f},  /// 13
		{+0.0f, -1.0f, +1.0f},  /// 14
		{+0.0f, -1.0f, +0.0f},  /// 15
		{+0.0f, -1.0f, -1.0f},  /// 16

		{-1.0f, +1.0f, +1.0f},  /// 17
		{-1.0f, +1.0f, +0.0f},  /// 18
		{-1.0f, +1.0f, -1.0f},  /// 19
		{-1.0f, +0.0f, +1.0f},  /// 20
		{-1.0f, +0.0f, +0.0f},  /// 21
		{-1.0f, +0.0f, -1.0f},  /// 22
		{-1.0f, -1.0f, +1.0f},  /// 23
		{-1.0f, -1.0f, +0.0f},  /// 24
		{-1.0f, -1.0f, -1.0f}   /// 25
	};

	const glm::mat4 g_ident = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotx2 = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, -1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotxp = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, -1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotxm = {
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_roty2 = {
		-1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotyp = {
		+0.0f, +0.0f, -1.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotym = {
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +1.0f, +0.0f, +0.0f,
		-1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotzp = {
		+0.0f, +1.0f, +0.0f, +0.0f,
		-1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rotzm = {
		+0.0f, -1.0f, +0.0f, +0.0f,
		+1.0f, +0.0f, +0.0f, +0.0f,
		+0.0f, +0.0f, +1.0f, +0.0f,
		+0.0f, +0.0f, +0.0f, +1.0f
	};
	const glm::mat4 g_rot_table[kNumStates] = {
		g_ident, g_rotxp,         g_rotx2,         g_rotxm,
		g_rotyp, g_rotxp*g_rotyp, g_rotx2*g_rotyp, g_rotxm*g_rotyp,
		g_roty2, g_rotxp*g_roty2, g_rotx2*g_roty2, g_rotxm*g_roty2,
		g_rotym, g_rotxp*g_rotym, g_rotx2*g_rotym, g_rotxm*g_rotym,
		g_rotzp, g_rotxp*g_rotzp, g_rotx2*g_rotzp, g_rotxm*g_rotzp,
		g_rotzm, g_rotxp*g_rotzm, g_rotx2*g_rotzm, g_rotxm*g_rotzm
	};

	const int g_mapxp[kNumCubes] = {
		2,  5,  8,  1,  4,  7,  0,  3,  6,
		11, 13, 16, 10,     15, 9,  12, 14,
		19, 22, 25, 18, 21, 24, 17, 20, 23
	};
	const int g_mapxm[kNumCubes] = {
		6,  3,  0,  7,  4,  1,  8,  5,  2,
		14, 12, 9,  15,     10, 16, 13, 11,
		23, 20, 17, 24, 21, 18, 25, 22, 19
	};
	const int g_mapyp[kNumCubes] = {
		17, 9,  0,  20, 12, 3,  23, 14, 6,
		18, 10, 1,  21,     4,  24, 15, 7,
		19, 11, 2,  22, 13, 5,  25, 16, 8
	};
	const int g_mapym[kNumCubes] = {
		2,  11, 19, 5,  13, 22, 8,  16, 25,
		1,  10, 18, 4,      21, 7,  15, 24,
		0,  9,  17, 3,  12, 20, 6,  14, 23
	};
	const int g_mapzp[kNumCubes] = {
		6,  7,  8, 14, 15, 16, 23, 24, 25,
		3,  4,  5, 12,     13, 20, 21, 22,
		0,  1,  2, 9,  10, 11, 17, 18, 19
	};
	const int g_mapzm[kNumCubes] = {
		17, 18, 19, 9,  10, 11, 0,  1,  2,
		20, 21, 22, 12,     13, 3,  4,  5,
		23, 24, 25, 14, 15, 16, 6,  7,  8
	};
	const int g_state_xp[kNumStates] = {
		1, 2, 3, 0, 5, 6, 7, 4, 9, 10, 11, 8, 13, 14, 15, 12, 17, 18, 19, 16, 21, 22, 23, 20
	};
	const int g_state_xm[kNumStates] = {
		3, 0, 1, 2, 7, 4, 5, 6, 11, 8, 9, 10, 15, 12, 13, 14, 19, 16, 17, 18, 23, 20, 21, 22
	};
	const int g_state_yp[kNumStates] = {
		4, 21, 14, 19, 8, 22, 2, 18, 12, 23, 6, 17, 0, 20, 10, 16, 5, 1, 13, 9, 7, 11, 15, 3
	};
	const int g_state_ym[kNumStates] = {
		12, 17, 6, 23, 0, 16, 10, 20, 4, 19, 14, 21, 8, 18, 2, 22, 15, 11, 7, 3, 13, 1, 5, 9
	};
	const int g_state_zp[kNumStates] = {
		16, 5, 22, 15, 19, 9, 23, 3, 18, 13, 20, 7, 17, 1, 21, 11, 10, 6, 2, 14, 0, 4, 8, 12
	};
	const int g_state_zm[kNumStates] = {
		20, 13, 18, 7, 21, 1, 17, 11, 22, 5, 16, 15, 23, 9, 19, 3, 0, 12, 8, 4, 10, 14, 2, 6
	};

	class StateChange {
	public:
		int symbol_;
		const char *text_;
		const int *rotation_map_;
		const int *state_map_;
		glm::vec3 axis_;
		int count_;
	};

	const int kUp = XK_Up;
	const int kDn = XK_Down;

	const StateChange g_change_a  = {'a', "A",  g_mapym, g_state_ym, {+0.0f, -1.0f, +0.0f}, kNumCubes};
	const StateChange g_change_d  = {'d', "D",  g_mapyp, g_state_yp, {+0.0f, +1.0f, +0.0f}, kNumCubes};
	const StateChange g_change_s  = {'s', "S",  g_mapzm, g_state_zm, {+0.0f, +0.0f, -1.0f}, kNumCubes};
	const StateChange g_change_w  = {'w', "W",  g_mapzp, g_state_zp, {+0.0f, +0.0f, +1.0f}, kNumCubes};
	const StateChange g_change_up = {kUp, "Up", g_mapxm, g_state_xm, {-1.0f, +0.0f, +0.0f}, kNumFaceCubes};
	const StateChange g_change_dn = {kDn, "Dn", g_mapxp, g_state_xp, {+1.0f, +0.0f, +0.0f}, kNumFaceCubes};
	const StateChange g_change_na = {0,   "",   nullptr, nullptr,    {+0.0f, +0.0f, +0.0f}, 0};

	const StateChange g_change_table[] = {
		g_change_a, g_change_d, g_change_s, g_change_w, g_change_up, g_change_dn
	};

	class MixUp {
	public:
		float prob_;
		const StateChange *change_;
		const MixUp *next_;
	};
	extern const MixUp g_mixup_all[];
	extern const MixUp g_mixup_up2[];
	extern const MixUp g_mixup_a2[];
	extern const MixUp g_mixup_w2[];
	extern const MixUp g_mixup_ad_sw[];
	extern const MixUp g_mixup_ad_updn[];
	extern const MixUp g_mixup_sw_updn[];
	const MixUp g_mixup_all[] = {
		{1.0f/9.0f, &g_change_up, g_mixup_ad_sw},
		{1.0f/9.0f, &g_change_dn, g_mixup_ad_sw},
		{1.0f/9.0f, &g_change_up, g_mixup_up2},
		{1.0f/9.0f, &g_change_a,  g_mixup_sw_updn},
		{1.0f/9.0f, &g_change_d,  g_mixup_sw_updn},
		{1.0f/9.0f, &g_change_a,  g_mixup_a2},
		{1.0f/9.0f, &g_change_w,  g_mixup_ad_updn},
		{1.0f/9.0f, &g_change_s,  g_mixup_ad_updn},
		{2.0f,      &g_change_w,  g_mixup_w2},
	};
	const MixUp g_mixup_up2[] = {
		{2.0f,      &g_change_up, g_mixup_ad_sw},
	};
	const MixUp g_mixup_a2[] = {
		{2.0f,      &g_change_a,  g_mixup_sw_updn},
	};
	const MixUp g_mixup_w2[] = {
		{2.0f,      &g_change_w,  g_mixup_ad_updn},
	};
	const MixUp g_mixup_ad_sw[] = {
		{1.0f/6.0f, &g_change_a,  g_mixup_sw_updn},
		{1.0f/6.0f, &g_change_d,  g_mixup_sw_updn},
		{1.0f/6.0f, &g_change_a,  g_mixup_a2},
		{1.0f/6.0f, &g_change_w,  g_mixup_ad_updn},
		{1.0f/6.0f, &g_change_s,  g_mixup_ad_updn},
		{2.0f,      &g_change_w,  g_mixup_w2},
	};
	const MixUp g_mixup_ad_updn[] = {
		{1.0f/6.0f, &g_change_up, g_mixup_ad_sw},
		{1.0f/6.0f, &g_change_dn, g_mixup_ad_sw},
		{1.0f/6.0f, &g_change_up, g_mixup_up2},
		{1.0f/6.0f, &g_change_a,  g_mixup_sw_updn},
		{1.0f/6.0f, &g_change_d,  g_mixup_sw_updn},
		{2.0f,      &g_change_a,  g_mixup_a2},
	};
	const MixUp g_mixup_sw_updn[] = {
		{1.0f/6.0f, &g_change_up, g_mixup_ad_sw},
		{1.0f/6.0f, &g_change_dn, g_mixup_ad_sw},
		{1.0f/6.0f, &g_change_up, g_mixup_up2},
		{1.0f/6.0f, &g_change_w,  g_mixup_ad_updn},
		{1.0f/6.0f, &g_change_s,  g_mixup_ad_updn},
		{2.0f,      &g_change_w,  g_mixup_w2},
	};

	class PieceState {
	public:
		int index_;
		int orient_;
	};

	class CubeState {
	public:
		PieceState pieces_[kNumCubes];
	};

	class SearchState {
	public:
		const StateChange *change_;
		CubeState state_;
	};

    class RenderImpl : public Render {
    public:
        RenderImpl() noexcept :
			ran_eng_(ran_dev_()),
			ran_turn_(0.0f, 1.0f) {

			for (int i = 0; i < kNumCubes; ++i) {
				auto& s = state_.pieces_[i];
				s.index_ = i;
				s.orient_ = 0;
			}
			/*state_.pieces_[3].orient_ = 7;
			state_.pieces_[5].orient_ = 7;
			state_.pieces_[20].orient_ = 7;
			state_.pieces_[22].orient_ = 7;*/
        };

        virtual ~RenderImpl() = default;

        int width_ = 0;
        int height_ = 0;
        GLuint vertex_buffer_ = 0;
        GLuint bevel_index_buffer_ = 0;
        GLuint face_index_buffer_[6] = {0};
        GLuint vertex_shader_ = 0;
        GLuint fragment_shader_ = 0;
        GLuint program_ = 0;
        GLuint model_mat_loc_ = 0;
        GLuint proj_view_mat_loc_ = 0;
        GLuint color_loc_ = 0;
        int num_bevel_indexes_ = 0;
        int num_face_indexes_ = 0;
        int frame_count_ = 0;
		CubeState state_;
		int rotate_counter_ = 0;
		const StateChange *state_change_ = nullptr;
		const StateChange *key_queue_ = nullptr;
		const MixUp *mix_up_ = nullptr;
		std::random_device ran_dev_;
		std::default_random_engine ran_eng_;
		std::uniform_real_distribution<> ran_turn_;
		int correctness_ = 0;
		std::vector<SearchState> moves_;
		int best_score_ = 0;

        virtual void init(
            int width,
            int height
        ) noexcept {
            width_ = width;
            height_ = height;

            //genTables();

            int num_vertex_floats = 3 * 8 * 6;
            num_bevel_indexes_ = 3 * 8 * 6;
            num_face_indexes_ = 3 * 2;

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);
            glEnable(GL_CULL_FACE);

            glGenBuffers(1, &vertex_buffer_);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
            glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat)*num_vertex_floats, g_cube_vertexes, GL_STATIC_DRAW);
            LOG("GL vertex=" << vertex_buffer_);

            glGenBuffers(1, &bevel_index_buffer_);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bevel_index_buffer_);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_bevel_indexes_, g_bevel_indexes, GL_STATIC_DRAW);
            LOG("GL bevel_index=" << bevel_index_buffer_);

            glGenBuffers(6, &face_index_buffer_[0]);
            for (int i = 0; i < 6; ++i) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face_index_buffer_[i]);
				auto face_indexes = &g_face_indexes[num_face_indexes_*i];
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_face_indexes_, face_indexes, GL_STATIC_DRAW);
				LOG("GL face_index[" << i << "]=" << face_index_buffer_[i]);
			}

            vertex_shader_ = agm::gl::compileShader(GL_VERTEX_SHADER, g_vertex_source);
            LOG("GL vertex_shader=" << vertex_shader_);
            fragment_shader_ = agm::gl::compileShader(GL_FRAGMENT_SHADER, g_fragment_source);
            LOG("GL fragment_shader=" << fragment_shader_);

            program_ = agm::gl::linkProgram(vertex_shader_, fragment_shader_);
            LOG("GL program=" << program_);

            glUseProgram(program_);
            model_mat_loc_ = glGetUniformLocation(program_, "model_mat");
            LOG("GL model_mat_loc=" << model_mat_loc_);
            proj_view_mat_loc_ = glGetUniformLocation(program_, "proj_view_mat");
            LOG("GL proj_view_mat_loc=" << proj_view_mat_loc_);
            color_loc_ = glGetUniformLocation(program_, "color");
            LOG("GL color_loc=" << color_loc_);
        }

        virtual void exit() noexcept {
            if (program_) {
                if (fragment_shader_) {
                    glDetachShader(program_, fragment_shader_);
                }
                if (vertex_shader_) {
                    glDetachShader(program_, vertex_shader_);
                }
                glDeleteProgram(program_);
                program_ = 0;
            }
            if (fragment_shader_) {
                glDeleteShader(fragment_shader_);
                fragment_shader_ = 0;
            }
            if (vertex_shader_) {
                glDeleteShader(vertex_shader_);
                vertex_shader_ = 0;
            }
            if (bevel_index_buffer_) {
                glDeleteBuffers(1, &bevel_index_buffer_);
                bevel_index_buffer_ = 0;
            }
            for (int i = 0; i < 6; ++i) {
				if (face_index_buffer_[i]) {
					glDeleteBuffers(1, &face_index_buffer_[i]);
					face_index_buffer_[i] = 0;
				}
			}
            if (vertex_buffer_) {
                glDeleteBuffers(1, &vertex_buffer_);
                vertex_buffer_ = 0;
            }
        }

        virtual void draw() noexcept {
			int rotate = updateDraw();

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glm::mat4 rot_mat;
			if (state_change_ && rotate) {
				float pi = (float) acos(-1.0f);
				float angle = - (2.0f*pi/4.0f)/*90 deg*/ / float(kFramesPerRotation) * float(rotate);
				rot_mat = std::move(glm::rotate(
					glm::mat4(),
					angle,
					state_change_->axis_
				));
			}

            glm::mat4 view_mat = std::move(glm::lookAt(
                glm::vec3(8.0f, 8.0f, 8.0f),  // camera location
                glm::vec3(0.0f, 0.0f, 0.0f),  // looking at
                glm::vec3(0.0f, 1.0f, 0.0f)   // up direction
            ));
            glm::mat4 proj_mat = std::move(glm::perspective(
                glm::radians(26.0f),  // fov
                float(width_)/float(height_),   // aspect ratio
                0.1f,  // near clipping plane
                30.0f  // far  clipping plane
            ));
            glm::mat4 proj_view_mat = proj_mat * view_mat;

            glViewport(0, 0, width_, height_);

            glUseProgram(program_);
            glUniformMatrix4fv(proj_view_mat_loc_, 1, GL_FALSE, &proj_view_mat[0][0]);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

			drawAllCubes(rot_mat);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableVertexAttribArray(0);
            glUseProgram(0);

			//captureFrame();
        }

        void drawAllCubes(
			const glm::mat4& rot_mat
		) noexcept {
			int ncubes = kNumCubes;
			if (state_change_) {
				ncubes = state_change_->count_;
			}
			drawSomeCubes(rot_mat, 0, ncubes);
			drawSomeCubes(g_ident, ncubes, kNumCubes);
		}

		void drawSomeCubes(
			const glm::mat4& rot_mat,
			int start,
			int stop
		) noexcept {
			for (int i = start; i < stop; ++i) {
				auto& s = state_.pieces_[i];
				glm::mat4 temp_mat = std::move(glm::translate(
					rot_mat,
					g_xyz[i]
				));
				glm::mat4 model_mat = temp_mat * g_rot_table[s.orient_];
				glUniformMatrix4fv(model_mat_loc_, 1, GL_FALSE, &model_mat[0][0]);
				draw1Cube();
			}
        }

        void draw1Cube() noexcept {
			GLfloat dark[] = {0.1f, 0.1f, 0.1f};
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bevel_index_buffer_);
			glUniform3fv(color_loc_, 1, &dark[0]);
			glDrawElements(GL_TRIANGLES, num_bevel_indexes_, GL_UNSIGNED_SHORT, nullptr);

			for (int i = 0; i < 6; ++i) {
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, face_index_buffer_[i]);
				glUniform3fv(color_loc_, 1, &g_colors[3*i]);
				glDrawElements(GL_TRIANGLES, num_face_indexes_, GL_UNSIGNED_SHORT, nullptr);
			}
        }

        virtual void resize(
            int width,
            int height
        ) noexcept {
            width_ = width;
            height_ = height;
        }

        void captureFrame() noexcept {
            if (frame_count_ >= 48*60) {
                return;
            }

            Png png_flipped;
            png_flipped.init(width_, height_);
            glReadPixels(0, 0, width_, height_, GL_RGB, GL_UNSIGNED_BYTE, png_flipped.data_);
            //png_flipped.write("flipped.png");

            Png png;
            png.init(width_, height_);
            auto src = png_flipped.data_ + (height_ - 1)*png_flipped.stride_;
            auto dst = png.data_;
            for (int i = 0; i < height_; ++i) {
                std::memcpy(dst, src, png.stride_);
                src -= png_flipped.stride_;
                dst += png.stride_;
            }

            std::stringstream ss;
            ss << "output/rubiks" << std::setfill('0') << std::setw(5) << frame_count_
                << std::setfill(' ') << std::setw(0) << ".png";
            LOG("ss=" << ss.str().c_str());
            png.write(ss.str().c_str());

            ++frame_count_;
        }

		virtual void keyPressed(
			int symbol
		) noexcept {
			symbol = tolower(symbol);
			if (symbol == ' ') {
				mix_up_ = mix_up_ ? nullptr : g_mixup_all;
			} else if (symbol == XK_Home) {
				mix_up_ = nullptr;
				searchSolution();
			} else {
				for (auto change = g_change_table; change->symbol_; ++change) {
					if (symbol == change->symbol_) {
						key_queue_ = change;
						break;
					}
				}
			}
		}

		int updateDraw() noexcept {
			if (rotate_counter_ < 0) {
				if (key_queue_ == nullptr) {
					if (mix_up_) {
						float ran_idx = ran_turn_(ran_eng_);
						for (auto dist = mix_up_; ; ++dist) {
							if (ran_idx < dist->prob_) {
								auto change = dist->change_;
								keyPressed(change->symbol_);
								mix_up_ = dist->next_;
								break;
							}
							ran_idx -= dist->prob_;
						}
					} else {
						if (moves_.size()) {
							auto head = moves_.begin();
							key_queue_ = head->change_;
							moves_.erase(head);
						}
					}
				}
				if (key_queue_) {
					rotate_counter_ = kFramesPerRotation;
					state_change_ = key_queue_;
					key_queue_ = nullptr;
					changeState(state_change_, state_, state_);
					correctness_ = checkCorrectness(state_);
				}
			}
			int rotate = std::max(0, rotate_counter_);
			rotate_counter_ = rotate - 1;
			return rotate;
		}

		void changeState(
			const StateChange *change,
			const CubeState& old_state,
			CubeState& new_state
		) noexcept {
			CubeState temp_state = old_state;
			int ncubes = change->count_;
			for (int i = 0; i < ncubes; ++i) {
				int new_idx = change->rotation_map_[i];
				auto& s = old_state.pieces_[new_idx];
				int moved_idx = s.orient_;
				int rot_idx = change->state_map_[moved_idx];
				auto& ns = temp_state.pieces_[i];
				ns.index_ = s.index_;
				ns.orient_ = rot_idx;
			}
			new_state = temp_state;
		}

		int checkCorrectness(
			const CubeState& state
		) noexcept {
			int new_correctness = 0;
			best_score_ = 0;
			if (new_correctness == best_score_) {
				best_score_ += 1;
				/// check if white center (10) is on the top face.
				/// we don't care about orientation.
				if (state.pieces_[10].index_ == 10) {
					new_correctness += 1;
				}
			}
			if (new_correctness == best_score_) {
				best_score_ += 4;
				/// check if the top edge pieces (1 9 11 18) are in the correct place
				/// and oriented correctly.
				int check_list[] = {1, 9, 11, 18};
				for (int i = 0; i < intsizeof(check_list)/intsizeof(int); ++i) {
					int idx = check_list[i];
					auto& s = state.pieces_[idx];
					if (s.index_ == idx && s.orient_ == 0) {
						++new_correctness;
					}
				}
			}
			if (new_correctness == best_score_) {
				best_score_ += 1;
				/// check if blue center (4) is on the right face.
				/// we don't care about orientation.
				if (state.pieces_[4].index_ == 4 && state.pieces_[10].index_ == 10) {
					new_correctness += 1;
				}
			}
			if (new_correctness == best_score_) {
				best_score_ += 4;
				/// check if the top corner pieces (0 2 17 19) are in the correct place
				/// and oriented correctly.
				int check_list[] = {0, 2, 17, 19};
				for (int i = 0; i < intsizeof(check_list)/intsizeof(int); ++i) {
					int idx = check_list[i];
					auto& s = state.pieces_[idx];
					if (s.index_ == idx && s.orient_ == 0) {
						++new_correctness;
					}
				}
			}
			if (new_correctness == best_score_) {
				best_score_ += 4;
				/// check if the middle edge pieces (3 5 20 22) are in the correct place
				/// and oriented correctly.
				int check_list[] = {3, 5, 20, 22};
				for (int i = 0; i < intsizeof(check_list)/intsizeof(int); ++i) {
					int idx = check_list[i];
					auto& s = state.pieces_[idx];
					if (s.index_ == idx && s.orient_ == 0) {
						++new_correctness;
					}
				}
			}
			return new_correctness;
		}

		void genTables() noexcept {
			genTable(g_rotxp);
			genTable(g_rotxm);
			genTable(g_rotyp);
			genTable(g_rotym);
			genTable(g_rotzp);
			genTable(g_rotzm);
		}

		void genTable(
			const glm::mat4& rot_mat
		) noexcept {
			for (int i = 0; i < kNumCubes; ++i) {
				auto& xyz = g_xyz[i];
				auto xyz1 = glm::vec4(xyz, 1.0f);
				auto rot4 = rot_mat * xyz1;
				auto rot3 = glm::vec3(rot4.x, rot4.y, rot4.z);

				int idx = -1;
				for (int k = 0; k < kNumCubes; ++k) {
					auto& xyz = g_xyz[k];
					if (rot3 == xyz) {
						idx = k;
						break;
					}
				}
				std::cout << idx << ", ";
			}
			std::cout << std::endl;
			for (int i = 0; i < kNumStates; ++i) {
				auto& entry = g_rot_table[i];
				auto result = rot_mat * entry;

				int idx = -1;
				for (int k = 0; k < kNumStates; ++k) {
					auto &entry = g_rot_table[k];
					if (result == entry) {
						idx = k;
						break;
					}
				}
				std::cout << idx << ", ";
			}
			std::cout << std::endl;
		}

		void searchSolution() noexcept {
			if (correctness_ == best_score_) {
				LOG("It's solved!");
				return;
			}

			LOG("starting correctness=" << correctness_);
			//logState(state_);
			bool found = false;
			int correctness = 0;
			const int kSearchDepth = 20;
			int depth = 1;
			for (; depth <= kSearchDepth; ++depth) {
				LOG("searching depth: " << depth);
				moves_.clear();
				SearchState search;
				search.change_ = g_change_table;
				search.state_ = state_;
				moves_.push_back(search);
				for (int i = 1; i < depth; ++i) {
					search.change_ = g_change_table;
					changeState(search.change_, search.state_, search.state_);
					moves_.push_back(search);
				}
				for(;;) {
					auto& move = moves_[depth-1];
					CubeState test;
					changeState(move.change_, move.state_, test);
					correctness = checkCorrectness(test);
					//auto s = buildSymbolList();
					//LOG("=TSC= list=" << s << "correctness=" << correctness);
					if (correctness > correctness_) {
						found = true;
						break;
					}

					bool try_again = false;
					for (int i = depth-1; i >= 0; --i) {
						auto& move = moves_[i];
						auto change = move.change_;
						++change;
						if (change->symbol_) {
							try_again = true;
						} else {
							change = g_change_table;
						}
						move.change_ = change;
						if (try_again) {
							for (++i; i < depth; ++i) {
								auto& move1 = moves_[i];
								changeState(move.change_, move.state_, move1.state_);
							}
							break;
						}
					}
					if (try_again == false) {
						break;
					}
				}
				if (found) {
					break;
				}
			}
			if (found) {
                auto s = buildSymbolList();
                LOG("Improve from " << correctness_ << " to " << correctness << " with moves: " << s);
			} else {
				moves_.clear();
				LOG("No improvement found in " << kSearchDepth << " moves.");
			}
		}

		std::string buildSymbolList() noexcept {
			std::string s;
			int n = moves_.size();
			for (int i = 0; i < n; ++i) {
				auto move = moves_[i];
				s += move.change_->text_;
				s += " ";
			}
			return s;
		}

		void logState(
			const CubeState& state
		) noexcept {
            std::string str;
            for (int i = 0; i < kNumCubes; ++i) {
				auto& s = state.pieces_[i];
				str += std::to_string(s.index_);
				str += ":";
				str += std::to_string(s.orient_);
				str += " ";
            }
            LOG(str);
		}
    };
}

Render::Render() noexcept {
}

Render::~Render() noexcept {
}

Render *Render::create() noexcept {
    auto impl = new(std::nothrow) RenderImpl;
    return impl;
}
