
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"decals.h"
#include	"weapons.h"
#include	"game.h"


#include "bm.h"

static int gMortarSpitSprite;

LINK_ENTITY_TO_CLASS( info_bigmomma, CInfoBM )

TYPEDESCRIPTION	CInfoBM::m_SaveData[] =
{
	DEFINE_FIELD( CInfoBM, m_preSequence, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CInfoBM, CPointEntity )

void CInfoBM::Spawn( void )
{
}

void CInfoBM::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		pev->scale = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "reachdelay" ) )
	{
		pev->speed = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "reachtarget" ) )
	{
		pev->message = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "reachsequence" ) )
	{
		pev->netname = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "presequence" ) )
	{
		m_preSequence = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

//=========================================================
// Mortar shot entity
//=========================================================
LINK_ENTITY_TO_CLASS( bmortar, CBMortar )

TYPEDESCRIPTION	CBMortar::m_SaveData[] =
{
	DEFINE_FIELD( CBMortar, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBMortar, CBaseEntity )

// ---------------------------------
//
// Mortar
//
// ---------------------------------
void CBMortar::Spray( const Vector &position, const Vector &direction, int count )
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, position );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( position.x );	// pos
		WRITE_COORD( position.y );	
		WRITE_COORD( position.z );	
		WRITE_COORD( direction.x );	// dir
		WRITE_COORD( direction.y );	
		WRITE_COORD( direction.z );	
		WRITE_SHORT( gMortarSpitSprite );	// model
		WRITE_BYTE ( count );			// count
		WRITE_BYTE ( 130 );			// speed
		WRITE_BYTE ( 80 );			// noise ( client will divide by 100 )
	MESSAGE_END();
}

// UNDONE: right now this is pretty much a copy of the squid spit with minor changes to the way it does damage
void CBMortar::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->classname = MAKE_STRING( "bmortar" );
	
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	SET_MODEL( ENT( pev ), "sprites/mommaspit.spr" );
	pev->frame = 0;
	pev->scale = 0.5f;

	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );

	m_maxFrame = MODEL_FRAMES( pev->modelindex ) - 1;
	pev->dmgtime = gpGlobals->time + 0.4f;
}

void CBMortar::Precache( void )
{
	PRECACHE_MODEL( "sprites/mommaspit.spr" );// spit projectile.
	gMortarSpitSprite = PRECACHE_MODEL( "sprites/mommaspout.spr" );// client side spittle.

	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
}

void CBMortar::Animate( void )
{
	SetNextThink( 0.1f );

	if( gpGlobals->time > pev->dmgtime )
	{
		pev->dmgtime = gpGlobals->time + 0.2f;
		Spray( pev->origin, -pev->velocity.Normalize(), 3 );
	}
	if( pev->frame++ )
	{
		if( pev->frame > m_maxFrame )
		{
			pev->frame = 0;
		}
	}
}

CBMortar *CBMortar::Shoot( edict_t *pOwner, Vector vecStart, Vector vecVelocity )
{
	CBMortar *pSpit = GetClassPtr( (CBMortar *)NULL );
	pSpit->Spawn();
	
	UTIL_SetOrigin( pSpit, vecStart );
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = pOwner;
	pSpit->pev->scale = 2.5f;
	pSpit->SetThink( &CBMortar::Animate );
	pSpit->SetNextThink( 0.1f );

	return pSpit;
}


void CBMortar::Touch( CBaseEntity *pOther )
{
	TraceResult tr;
	int iPitch;

	// splat sound
	iPitch = RANDOM_LONG( 90, 110 );

	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch );

	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch );
		break;
	}

	if( pOther->IsBSPModel() )
	{

		// make a splat on the wall
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10.0f, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_DecalTrace( &tr, DECAL_MOMMASPLAT );
	}
	else
	{
		tr.vecEndPos = pev->origin;
		tr.vecPlaneNormal = -1.0f * pev->velocity.Normalize();
	}

	// make some flecks
	Spray( tr.vecEndPos, tr.vecPlaneNormal, 24 );

	entvars_t *pevOwner = NULL;
	if( pev->owner )
		pevOwner = VARS(pev->owner);

	RadiusDamage( pev->origin, pev, pevOwner, gSkillData.bigmommaDmgBlast, gSkillData.bigmommaRadiusBlast, CLASS_NONE, DMG_ACID );
	UTIL_Remove( this );
}
