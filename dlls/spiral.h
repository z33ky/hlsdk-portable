
#ifndef SPIRAL_H
#define SPIRAL_H

#include "cbase.h"

void SpiralStreakSplash( const Vector &origin, const Vector &direction, int color, int count, int speed, int velocityRange );

// Spiral Effect
class CSpiral : public CBaseEntity
{
public:
	void Spawn( void );
	void Think( void );
	int ObjectCaps( void ) { return FCAP_DONT_SAVE; }
	static CSpiral *Create( const Vector &origin, float height, float radius, float duration );
};

#endif

