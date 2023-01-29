/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"

// special deathmatch shotgun spreads
#define VECTOR_CONE_DM_SHOTGUN	Vector( 0.08716, 0.04362, 0.00 )// 10 degrees by 5 degrees
#define VECTOR_CONE_DM_DOUBLESHOTGUN Vector( 0.17365, 0.04362, 0.00 ) // 20 degrees by 5 degrees

enum shotgun_e
{
	SHOTGUN_IDLE1 = 0,
	SHOTGUN_IDLE2,
	SHOTGUN_IDLE3,
	SHOTGUN_HOLSTER,
	SHOTGUN_DRAW,
	SHOTGUN_FIRE,
	SHOTGUN_FIRE2,
	SHOTGUN_RELOAD
};

class CShotgun : public CBasePlayerWeapon
{
public:
	void Spawn( void );
	void Precache( void );
	int iItemSlot( ) { return 1; }
	int GetItemInfo(ItemInfo *p);
	int AddToPlayer( CBasePlayer *pPlayer );

	void PrimaryAttack( void );
	void SecondaryAttack( void );
	BOOL Deploy( );
	void Holster( int skiplocal = 0 );
	void Reload( void );
	void WeaponIdle( void );
	int m_iShell;

	virtual BOOL UseDecrement( void )
	{
#if CLIENT_WEAPONS
		return TRUE;
#else
		return FALSE;
#endif
	}

private:
	unsigned short m_usDoubleFire;
	unsigned short m_usSingleFire;
};
LINK_ENTITY_TO_CLASS( weapon_shotgun, CShotgun )

void CShotgun::Spawn()
{
	Precache();
	m_iId = WEAPON_SHOTGUN;
	SET_MODEL( ENT( pev ), "models/w_shotgun.mdl" );

	m_iDefaultAmmo = SHOTGUN_DEFAULT_GIVE;

	FallInit();// get ready to fall
}

void CShotgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_shotgun.mdl" );
	PRECACHE_MODEL( "models/w_shotgun.mdl" );
	PRECACHE_MODEL( "models/p_shotgun.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );// shotgun shell

	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "weapons/dbarrel1.wav" );//shotgun
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );//shotgun

	PRECACHE_SOUND( "weapons/reload1.wav" );	// shotgun reload
	PRECACHE_SOUND( "weapons/reload3.wav" );	// shotgun reload

	//PRECACHE_SOUND( "weapons/sshell1.wav" );	// shotgun reload - played on client
	//PRECACHE_SOUND( "weapons/sshell3.wav" );	// shotgun reload - played on client
	
	PRECACHE_SOUND( "weapons/357_cock1.wav" ); // gun empty sound
	//PRECACHE_SOUND( "weapons/scock1.wav" );	// cock gun

	m_usSingleFire = PRECACHE_EVENT( 1, "events/shotgun1.sc" );
	m_usDoubleFire = PRECACHE_EVENT( 1, "events/shotgun2.sc" );
}

int CShotgun::AddToPlayer( CBasePlayer *pPlayer )
{
	if( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

int CShotgun::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "shotgun";
	p->iMaxAmmo1 = SHOTGUN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = SHOTGUN_MAX_CLIP;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_SHOTGUN;
	p->iWeight = SHOTGUN_WEIGHT;

	return 1;
}

BOOL CShotgun::Deploy()
{
	return DefaultDeploy( "models/v_shotgun.mdl", "models/p_shotgun.mdl", SHOTGUN_DRAW, "shotgun" );
}

void CShotgun::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5f;
	SendWeaponAnim( SHOTGUN_HOLSTER );
}

void CShotgun::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.15f );
		return;
	}

	if( m_iClip <= 0 )
	{
		Reload();
		if( m_iClip == 0 )
			PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	//m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		// altered deathmatch spread
		vecDir = m_pPlayer->FireBulletsPlayer( 4, vecSrc, vecAiming, VECTOR_CONE_DM_SHOTGUN, 2048, BULLET_PLAYER_SHOTGUN, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// regular old, untouched spread. 
		vecDir = m_pPlayer->FireBulletsPlayer( 6, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_SHOTGUN, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usSingleFire, 0.0, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

#if 0
	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
#endif

	float flSingleShotAnim = 0.95f;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flSingleShotAnim;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flSingleShotAnim;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flSingleShotAnim;
	if (m_iClip != 0)
		m_flTimeWeaponIdle += 3.6f;
}

void CShotgun::SecondaryAttack( void )
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3 && m_pPlayer->pev->watertype > CONTENT_FLYFIELD)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.15f );
		return;
	}

	if( m_iClip <= 1 )
	{
		Reload();
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip -= 2;

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	//m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	Vector vecDir;

#if CLIENT_DLL
	if( bIsMultiplayer() )
#else
	if( g_pGameRules->IsMultiplayer() )
#endif
	{
		// tuned for deathmatch
		vecDir = m_pPlayer->FireBulletsPlayer( 8, vecSrc, vecAiming, VECTOR_CONE_DM_DOUBLESHOTGUN, 2048, BULLET_PLAYER_SHOTGUN, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}
	else
	{
		// untouched default single player
		vecDir = m_pPlayer->FireBulletsPlayer( 12, vecSrc, vecAiming, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_SHOTGUN, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );
	}

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usDoubleFire, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

#if 0
	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );
#endif

	const float flDoubleShotAnim = 1.1;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + flDoubleShotAnim;
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + flDoubleShotAnim;

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + flDoubleShotAnim;
	if( m_iClip != 0 )
		m_flTimeWeaponIdle += 3.6f;
}

void CShotgun::Reload( void )
{
	int iResult = DefaultReload( SHOTGUN_MAX_CLIP, SHOTGUN_RELOAD, 111.0f / 36.0f );

	if(iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
	}
}

#if 0
void CShotgun::ItemPostFrame( void )
{
	if( m_flPumpTime && m_flPumpTime < gpGlobals->time )
	{
		// play pumping sound
		EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/scock1.wav", 1, ATTN_NORM, 0, 95 + RANDOM_LONG( 0, 0x1f ) );
		m_flPumpTime = 0;
	}

	CBasePlayerWeapon::ItemPostFrame();
}
#endif

void CShotgun::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle <  UTIL_WeaponTimeBase() )
	{
		if( m_iClip == 0 && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			Reload();
		}
		else
		{
			int iAnim;
			float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
			if( flRand <= 0.8f )
			{
				iAnim = SHOTGUN_IDLE1;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 89.0f / 30.0f );// * RANDOM_LONG( 2, 5 );
			}
			else if( flRand <= 0.95f )
			{
				iAnim = SHOTGUN_IDLE2;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 149.0f / 40.0f );
			}
			else
			{
				iAnim = SHOTGUN_IDLE3;
				m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 149.0f / 40.0f );
			}
			SendWeaponAnim( iAnim );
		}
	}
}

class CShotgunAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache();
		SET_MODEL( ENT( pev ), "models/w_shotgunammo.mdl" );
		CBasePlayerAmmo::Spawn();
	}
	void Precache( void )
	{
		PRECACHE_MODEL( "models/w_shotgunammo.mdl" );
		PRECACHE_SOUND( "items/9mmclip1.wav" );
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
#if 0
		if( pOther->GiveAmmo( AMMO_BUCKSHOTBOX_GIVE, "buckshot", BUCKSHOT_MAX_CARRY ) != -1 )
#else
		if( pOther->GiveAmmo( AMMO_SHOTGUN_GIVE, "shotgun", SHOTGUN_MAX_CARRY ) != -1 )
#endif
		{
			EMIT_SOUND( ENT( pev ), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
			return TRUE;
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( ammo_shotgun, CShotgunAmmo )
