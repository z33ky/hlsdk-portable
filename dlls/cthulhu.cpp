/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
#ifndef OEM_BUILD

//=========================================================
// Cthulhu
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"nodes.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"customentity.h"
#include	"weapons.h"
#include	"effects.h"
#include	"soundent.h"
#include	"decals.h"
#include	"explode.h"
#include	"func_break.h"
#include	"scripted.h"

#include "spiral.h"

//=========================================================
// Cthulhu Monster
//=========================================================
const float CTH_ATTACKDIST = 100.0;

// Cthulhu animation events
#define CTH_AE_SLASH_LEFT			1
//#define CTH_AE_BEAM_ATTACK_RIGHT	2		// No longer used
#define CTH_AE_LEFT_FOOT			3
#define CTH_AE_RIGHT_FOOT			4
//#define CTH_AE_STOMP				5		// Cthulhu does not stomp
#define CTH_AE_BREATHE				6


// Cthulhu is immune to any damage but this
//#define CTH_DAMAGE					(DMG_ENERGYBEAM|DMG_CRUSH|DMG_MORTAR|DMG_BLAST)
#define CTH_EYE_SPRITE_NAME		"sprites/gargeye1.spr"
#define CTH_BEAM_SPRITE_NAME		"sprites/xbeam3.spr"
#define CTH_BEAM_SPRITE2			"sprites/xbeam3.spr"
//#define CTH_STOMP_SPRITE_NAME		"sprites/gargeye1.spr"
//#define CTH_STOMP_BUZZ_SOUND		"weapons/mine_charge.wav"
#define CTH_FLAME_LENGTH			440
#define CTH_GIB_MODEL				"models/metalplategibs.mdl"

#define ATTN_CTH					(ATTN_NORM)

int gCthulhuGibModel = 0;

void CthSpawnExplosion( Vector center, float randomRange, float time, int magnitude );


class CCthulhu : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void KeyValue( KeyValueData *pkvd );
	void SetYawSpeed( void );
	int  Classify ( void );
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	BOOL CheckMeleeAttack1( float flDot, float flDist );		// Swipe
	BOOL CheckMeleeAttack2( float flDot, float flDist );		// Flames
	BOOL CheckRangeAttack1( float flDot, float flDist );		// Stomp attack
	void SetObjectCollisionBox( void )
	{
		//pev->absmin = pev->origin + Vector( -80, -80, 0 );
		//pev->absmax = pev->origin + Vector( 80, 80, 214 );
		pev->absmin = pev->origin + Vector( -160, -160, 0 );
		pev->absmax = pev->origin + Vector( 160, 160, 418 );
	}

	Schedule_t *GetScheduleOfType( int Type );
	void StartTask( Task_t *pTask );
	void RunTask( Task_t *pTask );

	void PrescheduleThink( void );

	void Killed( entvars_t *pevAttacker, int iGib );
	void DeathEffect( void );

	void EyeOff( void );
	void EyeOn( int level );
	void EyeUpdate( void );
	//void Leap( void );
	void FlameCreate( void );
	void FlameUpdate( void );
	void FlameControls( float angleX, float angleY );
	void FlameDestroy( void );
	inline BOOL FlameIsOn( void ) { return m_pFlame[0] != NULL; }

	void FlameDamage( Vector vecStart, Vector vecEnd, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType );

	virtual int		Save( CSave &save );
	virtual int		Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:
	static const char *pAttackHitSounds[];
	static const char *pBeamAttackSounds[];
	static const char *pAttackMissSounds[];
	static const char *pFootSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackSounds[];
	static const char *pBreatheSounds[];

	CBaseEntity* CthulhuCheckTraceHullAttack(float flDist, int iDamage, int iDmgType);

	CSprite		*m_pEyeGlow;		// Glow around the eyes
	CBeam		*m_pFlame[4];		// Flame beams

	int			m_eyeBrightness;	// Brightness target
	float		m_seeTime;			// Time to attack (when I see the enemy, I set this)
	float		m_flameTime;		// Time of next flame attack
	float		m_painSoundTime;	// Time of next pain sound
	float		m_streakTime;		// streak timer (don't send too many)
	float		m_flameX;			// Flame thrower aim
	float		m_flameY;			

	BOOL		m_bExplodeOnDeath;
};

LINK_ENTITY_TO_CLASS( monster_cthulhu, CCthulhu );

TYPEDESCRIPTION	CCthulhu::m_SaveData[] = 
{
	DEFINE_FIELD( CCthulhu, m_pEyeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( CCthulhu, m_eyeBrightness, FIELD_INTEGER ),
	DEFINE_FIELD( CCthulhu, m_seeTime, FIELD_TIME ),
	DEFINE_FIELD( CCthulhu, m_flameTime, FIELD_TIME ),
	DEFINE_FIELD( CCthulhu, m_streakTime, FIELD_TIME ),
	DEFINE_FIELD( CCthulhu, m_painSoundTime, FIELD_TIME ),
	DEFINE_ARRAY( CCthulhu, m_pFlame, FIELD_CLASSPTR, 4 ),
	DEFINE_FIELD( CCthulhu, m_flameX, FIELD_FLOAT ),
	DEFINE_FIELD( CCthulhu, m_flameY, FIELD_FLOAT ),
	DEFINE_FIELD( CCthulhu, m_bExplodeOnDeath, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CCthulhu, CBaseMonster );

const char *CCthulhu::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CCthulhu::pBeamAttackSounds[] = 
{
	"garg/gar_flameoff1.wav",
	"garg/gar_flameon1.wav",
	"garg/gar_flamerun1.wav",
};


const char *CCthulhu::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CCthulhu::pFootSounds[] = 
{
	"garg/gar_step1.wav",
	"garg/gar_step2.wav",
};


const char *CCthulhu::pIdleSounds[] = 
{
	"garg/gar_idle1.wav",
	"garg/gar_idle2.wav",
	"garg/gar_idle3.wav",
	"garg/gar_idle4.wav",
	"garg/gar_idle5.wav",
};


const char *CCthulhu::pAttackSounds[] = 
{
	"garg/gar_attack1.wav",
	"garg/gar_attack2.wav",
	"garg/gar_attack3.wav",
};

const char *CCthulhu::pAlertSounds[] = 
{
	"garg/gar_alert1.wav",
	"garg/gar_alert2.wav",
	"garg/gar_alert3.wav",
};

const char *CCthulhu::pPainSounds[] = 
{
	"garg/gar_pain1.wav",
	"garg/gar_pain2.wav",
	"garg/gar_pain3.wav",
};

const char *CCthulhu::pBreatheSounds[] = 
{
	"garg/gar_breathe1.wav",
	"garg/gar_breathe2.wav",
	"garg/gar_breathe3.wav",
};
//=========================================================
// AI Schedules Specific to this monster
//=========================================================
enum
{
	TASK_SOUND_ATTACK = LAST_COMMON_TASK + 1,
	TASK_FLAME_SWEEP,
};

Task_t	tlCthulhuFlame[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SOUND_ATTACK,		(float)0		},
	// { TASK_PLAY_SEQUENCE,		(float)ACT_SIGNAL1	},
	{ TASK_SET_ACTIVITY,		(float)ACT_MELEE_ATTACK2 },
	{ TASK_FLAME_SWEEP,			(float)4.5		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slCthulhuFlame[] =
{
	{ 
		tlCthulhuFlame,
		ARRAYSIZE ( tlCthulhuFlame ), 
		0,
		0,
		"CthulhuFlame"
	},
};


// primary melee attack
Task_t	tlCthulhuSwipe[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slCthulhuSwipe[] =
{
	{ 
		tlCthulhuSwipe,
		ARRAYSIZE ( tlCthulhuSwipe ), 
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"CthulhuSwipe"
	},
};


DEFINE_CUSTOM_SCHEDULES( CCthulhu )
{
	slCthulhuFlame,
	slCthulhuSwipe,
};

IMPLEMENT_CUSTOM_SCHEDULES( CCthulhu, CBaseMonster );


void CCthulhu::EyeOn( int level )
{
	m_eyeBrightness = level;	
}


void CCthulhu::EyeOff( void )
{
	m_eyeBrightness = 0;
}


void CCthulhu::EyeUpdate( void )
{
	if ( m_pEyeGlow )
	{
		m_pEyeGlow->pev->renderamt = UTIL_Approach( m_eyeBrightness, m_pEyeGlow->pev->renderamt, 26 );
		if ( m_pEyeGlow->pev->renderamt == 0 )
			m_pEyeGlow->pev->effects |= EF_NODRAW;
		else
			m_pEyeGlow->pev->effects &= ~EF_NODRAW;
		UTIL_SetOrigin( m_pEyeGlow, pev->origin );
	}
}


void CCthulhu :: FlameCreate( void )
{
	int			i;
	Vector		posGun, angleGun;
	TraceResult trace;

	UTIL_MakeVectors( pev->angles );
	
	// we only have one "flame", coming from the mouth
	for ( i = 0; i < 4; i += 2 )
	{
		if ( i < 2 )
			m_pFlame[i] = CBeam::BeamCreate( CTH_BEAM_SPRITE_NAME, 240 );
		else
			m_pFlame[i] = CBeam::BeamCreate( CTH_BEAM_SPRITE2, 140 );
		if ( m_pFlame[i] )
		{
			int attach = i%2;
			// attachment is 0 based in GetAttachment
			GetAttachment( attach+1, posGun, angleGun );

			Vector vecEnd = (gpGlobals->v_forward * CTH_FLAME_LENGTH) + posGun;
			UTIL_TraceLine( posGun, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->PointEntInit( trace.vecEndPos, entindex() );
			if ( i < 2 )
				m_pFlame[i]->SetColor( 255, 130, 90 );
			else
				m_pFlame[i]->SetColor( 0, 120, 255 );
			m_pFlame[i]->SetBrightness( 190 );
			m_pFlame[i]->SetFlags( BEAM_FSHADEIN );
			m_pFlame[i]->SetScrollRate( 20 );
			// attachment is 1 based in SetEndAttachment
			m_pFlame[i]->SetEndAttachment( attach + 2 );
			CSoundEnt::InsertSound( bits_SOUND_COMBAT, posGun, 384, 0.3 );
		}
	}
	EMIT_SOUND_DYN ( edict(), CHAN_BODY, pBeamAttackSounds[ 1 ], 1.0, ATTN_NORM, 0, PITCH_NORM );
	EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pBeamAttackSounds[ 2 ], 1.0, ATTN_NORM, 0, PITCH_NORM );
}


void CCthulhu :: FlameControls( float angleX, float angleY )
{
	if ( angleY < -180 )
		angleY += 360;
	else if ( angleY > 180 )
		angleY -= 360;

	if ( angleY < -45 )
		angleY = -45;
	else if ( angleY > 45 )
		angleY = 45;

	m_flameX = UTIL_ApproachAngle( angleX, m_flameX, 4 );
	m_flameY = UTIL_ApproachAngle( angleY, m_flameY, 8 );
	SetBoneController( 0, m_flameY );
	SetBoneController( 1, m_flameX );
}


void CCthulhu :: FlameUpdate( void )
{
	int				i;
	static float	offset[2] = { 60, -60 };
	TraceResult		trace;
	Vector			vecStart, angleGun;
	BOOL			streaks = FALSE;

	//for ( i = 0; i < 2; i++ )
	for ( i = 0; i < 1; i++ )
	{
		if ( m_pFlame[i] )
		{
			Vector vecAim = pev->angles;
			vecAim.x += m_flameX;
			vecAim.y += m_flameY;

			UTIL_MakeVectors( vecAim );

			GetAttachment( i+1, vecStart, angleGun );
			Vector vecEnd = vecStart + (gpGlobals->v_forward * CTH_FLAME_LENGTH); //  - offset[i] * gpGlobals->v_right;

			UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &trace );

			m_pFlame[i]->SetStartPos( trace.vecEndPos );
			m_pFlame[i+2]->SetStartPos( (vecStart * 0.6) + (trace.vecEndPos * 0.4) );

			if ( trace.flFraction != 1.0 && gpGlobals->time > m_streakTime )
			{
				SpiralStreakSplash( trace.vecEndPos, trace.vecPlaneNormal, 6, 20, 50, 400 );
				streaks = TRUE;
				UTIL_DecalTrace( &trace, DECAL_SMALLSCORCH1 + RANDOM_LONG(0,2) );
			}
			// RadiusDamage( trace.vecEndPos, pev, pev, gSkillData.gargantuaDmgFire, CLASS_ALIEN_MONSTER, DMG_BURN );
			FlameDamage( vecStart, trace.vecEndPos, pev, pev, gSkillData.gargantuaDmgFire, CLASS_ALIEN_MONSTER, DMG_POISON );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x1000 * (i + 2) );		// entity, attachment
				WRITE_COORD( vecStart.x );		// origin
				WRITE_COORD( vecStart.y );
				WRITE_COORD( vecStart.z );
				WRITE_COORD( RANDOM_FLOAT( 32, 48 ) );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 255 );	// G
				WRITE_BYTE( 255 );	// B
				WRITE_BYTE( 2 );	// life * 10
				WRITE_COORD( 0 ); // decay
			MESSAGE_END();
		}
	}
	if ( streaks )
		m_streakTime = gpGlobals->time;
}



void CCthulhu :: FlameDamage( Vector vecStart, Vector vecEnd, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType )
{
	CBaseEntity *pEntity = NULL;
	TraceResult	tr;
	float		flAdjustedDamage;
	Vector		vecSpot;

	Vector vecMid = (vecStart + vecEnd) * 0.5;

	float searchRadius = (vecStart - vecMid).Length();

	Vector vecAim = (vecEnd - vecStart).Normalize( );

	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere( pEntity, vecMid, searchRadius )) != NULL)
	{
		if ( pEntity->pev->takedamage != DAMAGE_NO )
		{
			// UNDONE: this should check a damage mask, not an ignore
			if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
			{// houndeyes don't hurt other houndeyes with their attack
				continue;
			}
			
			vecSpot = pEntity->BodyTarget( vecMid );
		
			float dist = DotProduct( vecAim, vecSpot - vecMid );
			if (dist > searchRadius)
				dist = searchRadius;
			else if (dist < -searchRadius)
				dist = searchRadius;
			
			Vector vecSrc = vecMid + dist * vecAim;

			UTIL_TraceLine ( vecSrc, vecSpot, dont_ignore_monsters, ENT(pev), &tr );

			if ( tr.flFraction == 1.0 || tr.pHit == pEntity->edict() )
			{// the explosion can 'see' this entity, so hurt them!
				// decrease damage for an ent that's farther from the flame.
				dist = ( vecSrc - tr.vecEndPos ).Length();

				//if (dist > 64)
				if (dist > 128)
				{
					flAdjustedDamage = flDamage - (dist - 128) * 0.4;
					if (flAdjustedDamage <= 0)
						continue;
				}
				else
				{
					flAdjustedDamage = flDamage;
				}

				// ALERT( at_console, "hit %s\n", STRING( pEntity->pev->classname ) );
				if (tr.flFraction != 1.0)
				{
					ClearMultiDamage( );
					pEntity->TraceAttack( pevInflictor, flAdjustedDamage, (tr.vecEndPos - vecSrc).Normalize( ), &tr, bitsDamageType );
					ApplyMultiDamage( pevInflictor, pevAttacker );
				}
				else
				{
					pEntity->TakeDamage ( pevInflictor, pevAttacker, flAdjustedDamage, bitsDamageType );
				}
			}
		}
	}
}


void CCthulhu :: FlameDestroy( void )
{
	int i;

	EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pBeamAttackSounds[ 0 ], 1.0, ATTN_NORM, 0, PITCH_NORM );
	//for ( i = 0; i < 4; i++ )
	for ( i = 0; i < 4; i += 2 )
	{
		if ( m_pFlame[i] )
		{
			UTIL_Remove( m_pFlame[i] );
			m_pFlame[i] = NULL;
		}
	}
}


void CCthulhu :: PrescheduleThink( void )
{
	if ( !HasConditions( bits_COND_SEE_ENEMY ) )
	{
		m_seeTime = gpGlobals->time + 5;
		EyeOff();
	}
	else
		EyeOn( 200 );
	
	EyeUpdate();
}


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CCthulhu :: Classify ( void )
{
	return m_iClass?m_iClass:CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CCthulhu :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 60;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_WALK:
	case ACT_RUN:
		ys = 60;
		break;

	default:
		ys = 60;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// Spawn
//=========================================================
void CCthulhu :: Spawn()
{
	Precache( );

	if (pev->model)
		SET_MODEL(ENT(pev), STRING(pev->model)); //LRC
	else
		SET_MODEL(ENT(pev), "models/cthulhu.mdl");
	//UTIL_SetSize( pev, Vector( -64, -64, 0 ), Vector( 64, 64, 512 ) );
	UTIL_SetSize( pev, Vector( -64, -64, 0 ), Vector( 64, 64, 512 ) );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	if (pev->health == 0)
		pev->health			= gSkillData.gargantuaHealth;
	//pev->view_ofs		= Vector ( 0, 0, 96 );// taken from mdl file
	//m_flFieldOfView		= -0.2;// width of forward view cone ( as a dotproduct result )
	m_flFieldOfView		= 0.3;// width of forward view cone ( as a dotproduct result )
							  // a more narrow field of view...
	m_MonsterState		= MONSTERSTATE_NONE;
	m_bExplodeOnDeath	= FALSE;

	MonsterInit();

	// override this, Cthulhu has long look range...
	m_flDistLook		= 4096.0;

	m_pEyeGlow = CSprite::SpriteCreate( CTH_EYE_SPRITE_NAME, pev->origin, FALSE );
	m_pEyeGlow->SetTransparency( kRenderGlow, 255, 255, 255, 0, kRenderFxNoDissipation );
	m_pEyeGlow->SetAttachment( edict(), 1 );
	EyeOff();
	m_seeTime = gpGlobals->time + 5;
	m_flameTime = gpGlobals->time + 2;
}


//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CCthulhu :: Precache()
{
	int i;

	if (pev->model)
		PRECACHE_MODEL((char*)STRING(pev->model)); //LRC
	else
		PRECACHE_MODEL("models/cthulhu.mdl");
	
	PRECACHE_MODEL( CTH_EYE_SPRITE_NAME );
	PRECACHE_MODEL( CTH_BEAM_SPRITE_NAME );
	PRECACHE_MODEL( CTH_BEAM_SPRITE2 );
	gCthulhuGibModel = PRECACHE_MODEL( CTH_GIB_MODEL );

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pBeamAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pBeamAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pFootSounds ); i++ )
		PRECACHE_SOUND((char *)pFootSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pBreatheSounds ); i++ )
		PRECACHE_SOUND((char *)pBreatheSounds[i]);
}	

void CCthulhu :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "blowup"))
	{
		int flag = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
		if ( flag ) m_bExplodeOnDeath = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CCthulhu::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType )
{
	ALERT( at_aiconsole, "CCthulhu::TraceAttack\n");

	if ( !IsAlive() )
	{
		CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
		return;
	}

	// UNDONE: Hit group specific damage?
	if ( bitsDamageType && m_painSoundTime < gpGlobals->time )
	{
		EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_CTH, 0, PITCH_NORM );
		m_painSoundTime = gpGlobals->time + RANDOM_FLOAT( 2.5, 4 );
	}

	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );

}



int CCthulhu::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	ALERT( at_aiconsole, "CCthulhu::TakeDamage\n");

	if ( IsAlive() )
	{
		if ( bitsDamageType & DMG_BLAST )
			SetConditions( bits_COND_LIGHT_DAMAGE );
	}

	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}


void CCthulhu::DeathEffect( void )
{
	UTIL_MakeVectors(pev->angles);
	Vector deathPos = pev->origin + gpGlobals->v_forward * 100;

	// Create a spiral of streaks
	CSpiral::Create( deathPos, (pev->absmax.z - pev->absmin.z) * 0.6, 125, 1.5 );

	Vector position = pev->origin;
	position.z += 32;
	if (m_bExplodeOnDeath)
	{
		int i;
		for ( i = 0; i < 7; i+=2 )
		{
			CthSpawnExplosion( position, 70, (i * 0.3), 60 + (i*20) );
			position.z += 15;
		}
	}

	CBaseEntity *pSmoker = CBaseEntity::Create( "env_smoker", pev->origin, g_vecZero, NULL );
	pSmoker->pev->health = 1;	// 1 smoke balls
	pSmoker->pev->scale = 46;	// 4.6X normal size
	pSmoker->pev->dmg = 0;		// 0 radial distribution
	pSmoker->SetNextThink( 2.5 );	// Start in 2.5 seconds
}


void CCthulhu::Killed( entvars_t *pevAttacker, int iGib )
{
	EyeOff();
	UTIL_Remove( m_pEyeGlow );
	m_pEyeGlow = NULL;
	CBaseMonster::Killed( pevAttacker, GIB_NEVER );
}

//=========================================================
// CheckMeleeAttack1
// Cthulhu swipe attack
// 
//=========================================================
BOOL CCthulhu::CheckMeleeAttack1( float flDot, float flDist )
{
//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	if (flDot >= 0.7)
	{
		if (flDist <= CTH_ATTACKDIST)
			return TRUE;
	}
	return FALSE;
}


// Flame thrower madness!
BOOL CCthulhu::CheckMeleeAttack2( float flDot, float flDist )
{
//	ALERT(at_aiconsole, "CheckMelee(%f, %f)\n", flDot, flDist);

	if ( gpGlobals->time > m_flameTime )
	{
		if (flDot >= 0.8 && flDist > CTH_ATTACKDIST)
		{
			if ( flDist <= CTH_FLAME_LENGTH )
				return TRUE;
		}
	}
	return FALSE;
}


//=========================================================
// CheckRangeAttack1
// flDot is the cos of the angle of the cone within which
// the attack can occur.
//=========================================================
BOOL CCthulhu::CheckRangeAttack1( float flDot, float flDist )
{
	//if ( gpGlobals->time > m_seeTime )
	//{
	//	if (flDot >= 0.7 && flDist > CTH_ATTACKDIST)
	//	{
	//			return TRUE;
	//	}
	//}
	return FALSE;
}




//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CCthulhu::HandleAnimEvent(MonsterEvent_t *pEvent)
{
	switch( pEvent->event )
	{
	case CTH_AE_SLASH_LEFT:
		{
			// HACKHACK!!!
			CBaseEntity *pHurt = CthulhuCheckTraceHullAttack( CTH_ATTACKDIST + 10.0, gSkillData.gargantuaDmgSlash, DMG_SLASH );
			if (pHurt)
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = -30; // pitch
					pHurt->pev->punchangle.y = -30;	// yaw
					pHurt->pev->punchangle.z = 30;	// roll
					//UTIL_MakeVectors(pev->angles);	// called by CheckTraceHullAttack
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 50 + RANDOM_LONG(0,15) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( edict(), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 50 + RANDOM_LONG(0,15) );

			Vector forward;
			UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );
		}
		break;

	case CTH_AE_RIGHT_FOOT:
	case CTH_AE_LEFT_FOOT:
		UTIL_ScreenShake( pev->origin, 4.0, 3.0, 1.0, 750 );
		EMIT_SOUND_DYN ( edict(), CHAN_BODY, pFootSounds[ RANDOM_LONG(0,ARRAYSIZE(pFootSounds)-1) ], 1.0, ATTN_CTH, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		break;

	case CTH_AE_BREATHE:
		if ( !(pev->spawnflags & SF_MONSTER_GAG) || m_MonsterState != MONSTERSTATE_IDLE)
			EMIT_SOUND_DYN ( edict(), CHAN_VOICE, pBreatheSounds[ RANDOM_LONG(0,ARRAYSIZE(pBreatheSounds)-1) ], 1.0, ATTN_CTH, 0, PITCH_NORM + RANDOM_LONG(-10,10) );
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}


//=========================================================
// CheckTraceHullAttack - expects a length to trace, amount 
// of damage to do, and damage type. Returns a pointer to
// the damaged entity in case the monster wishes to do
// other stuff to the victim (punchangle, etc)
// Used for many contact-range melee attacks. Bites, claws, etc.

// Overridden for Cthulhu because his swing starts lower as
// a percentage of his height (otherwise he swings over the
// players head)
//=========================================================
CBaseEntity* CCthulhu::CthulhuCheckTraceHullAttack(float flDist, int iDamage, int iDmgType)
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += 64;
	//vecStart.z += 128;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * flDist) - (gpGlobals->v_up * flDist * 0.3);

	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		if ( iDamage > 0 )
		{
			pEntity->TakeDamage( pev, pev, iDamage, iDmgType );
		}

		return pEntity;
	}

	return NULL;
}


Schedule_t *CCthulhu::GetScheduleOfType( int Type )
{
	// HACKHACK - turn off the flames if they are on and cthulhu goes scripted / dead
	if ( FlameIsOn() )
		FlameDestroy();

	switch( Type )
	{
		case SCHED_MELEE_ATTACK2:
			return slCthulhuFlame;
		case SCHED_MELEE_ATTACK1:
			return slCthulhuSwipe;
		break;
	}

	return CBaseMonster::GetScheduleOfType( Type );
}


void CCthulhu::StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_FLAME_SWEEP:
		FlameCreate();
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		m_flameTime = gpGlobals->time + 6;
		m_flameX = 0;
		m_flameY = 0;
		break;

	case TASK_SOUND_ATTACK:
		if ( RANDOM_LONG(0,100) < 30 )
			EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_CTH, 0, PITCH_NORM );
		TaskComplete();
		break;

	// allow a scripted_action to make cthulhu shoot flames.
	case TASK_PLAY_SCRIPT:
		if ( m_pCine->IsAction() && m_pCine->m_fAction == 3)
		{
			FlameCreate();
			m_flWaitFinished = gpGlobals->time + 4.5;
			m_flameTime = gpGlobals->time + 6;
			m_flameX = 0;
			m_flameY = 0;
		}
		CBaseMonster::RunTask( pTask );
		break;

	case TASK_DIE:
		m_flWaitFinished = gpGlobals->time + 1.6;
		DeathEffect();
		// FALL THROUGH
	default: 
		CBaseMonster::StartTask( pTask );
		break;
	}
}

//=========================================================
// RunTask
//=========================================================
void CCthulhu::RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_DIE:
		if ( m_bExplodeOnDeath && gpGlobals->time > m_flWaitFinished )
		{
			pev->renderfx = kRenderFxExplode;
			pev->rendercolor.x = 255;
			pev->rendercolor.y = 0;
			pev->rendercolor.z = 0;
			StopAnimation();
			SetNextThink( 0.15 );
			SetThink( &CCthulhu::SUB_Remove );
			int i;
			int parts = MODEL_FRAMES( gCthulhuGibModel );
			for ( i = 0; i < 10; i++ )
			{
				CGib *pGib = GetClassPtr( (CGib *)NULL );

				pGib->Spawn( CTH_GIB_MODEL );
				
				int bodyPart = 0;
				if ( parts > 1 )
					bodyPart = RANDOM_LONG( 0, pev->body-1 );

				pGib->pev->body = bodyPart;
				pGib->m_bloodColor = BLOOD_COLOR_YELLOW;
				pGib->m_material = matNone;
				pGib->pev->origin = pev->origin;
				pGib->pev->velocity = UTIL_RandomBloodVector() * RANDOM_FLOAT( 300, 500 );
				pGib->SetNextThink( 1.25 );
				pGib->SetThink( &CGib::SUB_FadeOut );
			}
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
				WRITE_BYTE( TE_BREAKMODEL);

				// position
				WRITE_COORD( pev->origin.x );
				WRITE_COORD( pev->origin.y );
				WRITE_COORD( pev->origin.z );

				// size
				WRITE_COORD( 200 );
				WRITE_COORD( 200 );
				WRITE_COORD( 128 );

				// velocity
				WRITE_COORD( 0 ); 
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );

				// randomization
				WRITE_BYTE( 200 ); 

				// Model
				WRITE_SHORT( gCthulhuGibModel );	//model id#

				// # of shards
				WRITE_BYTE( 50 );

				// duration
				WRITE_BYTE( 20 );// 3.0 seconds

				// flags

				WRITE_BYTE( BREAK_FLESH );
			MESSAGE_END();

			return;
		}
		else
			CBaseMonster::RunTask(pTask);
		break;

	case TASK_PLAY_SCRIPT:
		if (!m_pCine->IsAction() || m_pCine->m_fAction != 3)
		{
			CBaseMonster::RunTask( pTask );
			break;
		}
		else
		{
			if (m_fSequenceFinished)
			{
				FlameDestroy();
				FlameControls( 0, 0 );
				SetBoneController( 0, 0 );
				SetBoneController( 1, 0 );
				m_pCine->SequenceDone( this );
				break;
			}
			//if not finished, carry on into task_flame_sweep!
		}
	case TASK_FLAME_SWEEP:
		if ( gpGlobals->time > m_flWaitFinished )
		{
			FlameDestroy();
			TaskComplete();
			FlameControls( 0, 0 );
			SetBoneController( 0, 0 );
			SetBoneController( 1, 0 );
		}
		else
		{
			BOOL cancel = FALSE;

			Vector angles = g_vecZero;

			FlameUpdate();
			CBaseEntity *pEnemy;
			if (m_pCine) // LRC- are we obeying a scripted_action?
				pEnemy = m_hTargetEnt;
			else
				pEnemy = m_hEnemy;
			if ( pEnemy )
			{
				Vector org = pev->origin;
				//org.z += 64;
				org.z += 128;
				Vector dir = pEnemy->BodyTarget(org) - org;
				angles = UTIL_VecToAngles( dir );
				angles.x = -angles.x;
				angles.y -= pev->angles.y;
				//if ( dir.Length() > 400 )
				if ( dir.Length() > 600 )
					cancel = TRUE;
			}
			if ( fabs(angles.y) > 60 )
				cancel = TRUE;
			
			if ( cancel )
			{
				m_flWaitFinished -= 0.5;
				m_flameTime -= 0.5;
			}
			// FlameControls( angles.x + 2 * sin(gpGlobals->time*8), angles.y + 28 * sin(gpGlobals->time*8.5) );
			FlameControls( angles.x, angles.y );
		}
		break;

	default:
		CBaseMonster::RunTask( pTask );
		break;
	}
}



// HACKHACK Cut and pasted from explode.cpp
void CthSpawnExplosion( Vector center, float randomRange, float time, int magnitude )
{
	KeyValueData	kvd;
	char			buf[128];

	center.x += RANDOM_FLOAT( -randomRange, randomRange );
	center.y += RANDOM_FLOAT( -randomRange, randomRange );

	CBaseEntity *pExplosion = CBaseEntity::Create( "env_explosion", center, g_vecZero, NULL );
	sprintf( buf, "%3d", magnitude );
	kvd.szKeyName = "iMagnitude";
	kvd.szValue = buf;
	pExplosion->KeyValue( &kvd );
	pExplosion->pev->spawnflags |= SF_ENVEXPLOSION_NODAMAGE;

	pExplosion->Spawn();
	pExplosion->SetThink( &CBaseEntity::SUB_CallUseToggle );
	pExplosion->SetNextThink( time );
}



#endif