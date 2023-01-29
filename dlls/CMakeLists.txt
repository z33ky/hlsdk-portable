#
# Copyright (c) 2015 Pavlo Lavrenenko
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

cmake_minimum_required(VERSION 2.8.12)
project (SVDLL)

set (SVDLL_LIBRARY server)

if(NOT MSVC)
	add_compile_options(-fno-exceptions) # GCC/Clang flag
	add_compile_options(-Wno-invalid-offsetof) # GCC/Clang flag
	add_compile_options(-fvisibility=hidden) # GCC/Clang flag
	add_definitions(-D_LINUX) # It seems enough for all non-Win32 systems
	add_definitions(-Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -D_snprintf=snprintf -D_vsnprintf=vsnprintf )
else()
	add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif()

set (SVDLL_SOURCES
	agrunt.cpp
	airtank.cpp
	aflock.cpp
	alias.cpp
	animating.cpp
	animation.cpp
	apache.cpp
	barnacle.cpp
	barney.cpp
	bigmomma.cpp
	bm.cpp
	bloater.cpp
	bmodels.cpp
	buttons.cpp
	cbase.cpp
	client.cpp
	combat.cpp
	controller.cpp
	defaultai.cpp
	doors.cpp
	effects.cpp
	explode.cpp
	flyingmonster.cpp
	func_break.cpp
	func_tank.cpp
	game.cpp
	gamerules.cpp
	gargantua.cpp
	spiral.cpp
	genericmonster.cpp
	ggrenade.cpp
	globals.cpp
	gman.cpp
	h_ai.cpp
	h_cine.cpp
	h_cycler.cpp
	h_export.cpp
	hassassin.cpp
	headcrab.cpp
	healthkit.cpp
	hornet.cpp
	ichthyosaur.cpp
	islave_deamon.cpp
	items.cpp
	leech.cpp
	lights.cpp
	locus.cpp
	maprules.cpp
	monstermaker.cpp
	monsters.cpp
	monsterstate.cpp
	mortar.cpp
	movewith.cpp
	multiplay_gamerules.cpp
	nihilanth.cpp
	nodes.cpp
	observer.cpp
	osprey.cpp
	pathcorner.cpp
	plane.cpp
	plats.cpp
	player.cpp
	playermonster.cpp
	rat.cpp
	roach.cpp
	schedule.cpp
	scientist.cpp
	scripted.cpp
	shotgun.cpp
	singleplay_gamerules.cpp
	skill.cpp
	sound.cpp
	soundent.cpp
	spectator.cpp
	squadmonster.cpp
	subs.cpp
	talkmonster.cpp
	teamplay_gamerules.cpp
	tempmonster.cpp
	tentacle.cpp
	triggers.cpp
	turret.cpp
	util.cpp
	weapons.cpp
	world.cpp
	xen.cpp
	zombie.cpp

	Chicken.cpp
	ClimbingMonster.cpp
	Cow.cpp
	Cthonian.cpp
	Dagger.cpp
	DeepOne.cpp
	DimensionalShambler.cpp
	DreadName.cpp
	DunwichHorror.cpp
	FormlessSpawn.cpp
	Gangster.cpp
	Ghoul.cpp
	GreatRace.cpp
	HuntingHorror.cpp
	NightGaunt.cpp
	PowderOfIbn.cpp
	Priest.cpp
	Rifle.cpp
	SerpentMan.cpp
	SerpentStaff.cpp
	SirHenry.cpp
	Snake.cpp
	Stukabat.cpp
	SwordCane.cpp
	TRex.cpp
	TommyGun.cpp
	Yodan.cpp
	alien_egg.cpp
	book.cpp
	butler.cpp
	charm.cpp
	civilian.cpp
	cthulhu.cpp
	cultist.cpp
	drainlife.cpp
	dynamite.cpp
	eihortvictim.cpp
	elder_sign.cpp
	furniture.cpp
	golem.cpp
	hellhound.cpp
	hologram.cpp
	kingpin.cpp
	laserspot.cpp
	lightning_gun.cpp
	molotov.cpp
	monster_PowderOfIbn.cpp
	monster_dynamite.cpp
	monster_molotov.cpp
	orderly.cpp
	policeman.cpp
	ranulf.cpp
	revolver.cpp
	rlyeh_seal.cpp
	servitor.cpp
	shrivelling.cpp
	spider.cpp
	stukagrenade.cpp
	szlachta.cpp
	teleport.cpp
	tindalos.cpp
	werewolf.cpp
	wolf.cpp
	wraith.cpp

	../pm_shared/pm_debug.c
	../pm_shared/pm_math.c
	../pm_shared/pm_shared.c
)

include_directories (. wpn_shared ../common ../engine ../pm_shared ../game_shared ../public)

if(MSVC)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		hl.def)
endif()

if(USE_VOICEMGR)
	set(SVDLL_SOURCES
		${SVDLL_SOURCES}
		../game_shared/voice_gamemgr.cpp)
else()
	add_definitions(-DNO_VOICEGAMEMGR)
endif()

add_library (${SVDLL_LIBRARY} SHARED ${SVDLL_SOURCES})

set_target_properties (${SVDLL_LIBRARY} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Android")
	set(SVDLL_NAME "${SERVER_LIBRARY_NAME}")

	set_target_properties(${SVDLL_LIBRARY} PROPERTIES
		OUTPUT_NAME ${SVDLL_NAME}
		PREFIX "")
endif()

if(MSVC)
	set_property(TARGET ${SVDLL_LIBRARY} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

install( TARGETS ${SVDLL_LIBRARY}
	DESTINATION "${GAMEDIR}/${SERVER_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)
