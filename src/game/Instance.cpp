//----------------------------------------------------------------
// Instance.cpp
//
// Copyright 2002-2005 Raven Software
//----------------------------------------------------------------




#include "Instance.h"

rvInstance::rvInstance( int id, bool deferPopulate ) {
	instanceID = id;
	spawnInstanceID = id;

	gameLocal.AddClipWorld( id );

	mapEntityNumbers = NULL;
	numMapEntities = 0;

	initialSpawnCount = idGameLocal::INITIAL_SPAWN_COUNT;

	if ( !deferPopulate ) {
		Populate();
	}
}

rvInstance::~rvInstance() {
	if ( mapEntityNumbers ) {
		delete[] mapEntityNumbers;
		mapEntityNumbers = NULL;
	}
	gameLocal.RemoveClipWorld( instanceID );
}

void rvInstance::Populate( int serverChecksum ) {
	gameState_t currentState = gameLocal.GameState();
	
	// disable the minSpawnIndex lock out
	int latchMinSpawnIndex = gameLocal.minSpawnIndex;
	gameLocal.minSpawnIndex = MAX_CLIENTS;

	if ( currentState != GAMESTATE_STARTUP ) {
		gameLocal.SetGameState( GAMESTATE_RESTART );
	}

	if ( gameLocal.isServer ) {
		// When populating on a server, record the entity numbers	
		numMapEntities = gameLocal.GetNumMapEntities();

		// mwhitlock: Dynamic memory consolidation
		RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_LEVEL);
		if ( mapEntityNumbers ) {
			delete[] mapEntityNumbers;
		}
		mapEntityNumbers = new unsigned short[ numMapEntities ];
		RV_POP_HEAP();

		memset( mapEntityNumbers, -1, sizeof( unsigned short ) * numMapEntities );

		// read the index we should start populating at as transmitted by the server
		gameLocal.firstFreeIndex = gameLocal.GetStartingIndexForInstance( instanceID );
		//common->Printf( "pos: get starting index for instance %d sets firstFreeIndex to %d\n", instanceID, gameLocal.firstFreeIndex );

		// remember the spawnCount ahead of time, so that the client can accurately reconstruct its spawnIds
		gameLocal.SpawnMapEntities( spawnInstanceID, NULL, mapEntityNumbers, &initialSpawnCount );

		// only build the message in MP
		if ( gameLocal.isMultiplayer ) {
			BuildInstanceMessage();

			// force joins of anyone in our instance so they get potentially new map entitynumbers
			for( int i = 0; i < MAX_CLIENTS; i++ ) {
				PACIFIER_UPDATE;
				idPlayer* player = (idPlayer*)gameLocal.entities[ i ];
				if( player && player->GetInstance() == instanceID ) {
					networkSystem->ServerSendReliableMessage( player->entityNumber, mapEntityMsg );
				}
			}
		}
	} else {
		bool proto69 = ( gameLocal.GetCurrentDemoProtocol() == 69 );

		// When populating on a client, spawn the map using existing numbers		
		// check for good state
		// OK to spawn w/o specific entity numbers if we're in the startup process.  Otherwise, 
		// we need entity numbers from the server.
		// TTimo: only valid for backward 1.2 playback now
		assert( !proto69 || ( mapEntityNumbers || ( instanceID == 0 && gameLocal.GameState() == GAMESTATE_STARTUP ) ) );
		
		if ( !proto69 ) {
			// have the client produce a log of the entity layout so we can match it with the server's
			// this is also going to be used to issue the EV_FindTargets below
			if ( mapEntityNumbers ) {
				delete []mapEntityNumbers;
			}
			numMapEntities = gameLocal.GetNumMapEntities();
			RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_LEVEL);
			mapEntityNumbers = new unsigned short[ numMapEntities ];
			RV_POP_HEAP();
			memset( mapEntityNumbers, -1, sizeof( unsigned short ) * numMapEntities );
		}

		gameLocal.firstFreeIndex = gameLocal.GetStartingIndexForInstance( instanceID );

		gameLocal.SetSpawnCount( initialSpawnCount ); // that was transmitted through the instance msg
		if ( proto69 ) {
			gameLocal.SpawnMapEntities( spawnInstanceID, mapEntityNumbers, NULL );
		} else {
			gameLocal.SpawnMapEntities( spawnInstanceID, NULL, mapEntityNumbers );
			LittleRevBytes( mapEntityNumbers, sizeof(unsigned short ), numMapEntities ); //DAJ
			int checksum = MD5_BlockChecksum( mapEntityNumbers, sizeof( unsigned short ) * numMapEntities );
			if ( serverChecksum != 0 && checksum != serverChecksum ) {
				common->Error( "client side map populate checksum ( 0x%x ) doesn't match server's ( 0x%x )", checksum, serverChecksum );
			}
		}
	}


	for ( int i = 0; i < numMapEntities; i++ ) {
		if ( mapEntityNumbers[ i ] < 0 || mapEntityNumbers[ i ] >= MAX_GENTITIES ) {
			continue;
		}

		if ( (i % 100) == 0 ) {
			PACIFIER_UPDATE;
		}

		idEntity* ent = gameLocal.entities[ mapEntityNumbers[ i ] ];

		if ( ent ) {
			ent->PostEventMS( &EV_FindTargets, 0 );
			ent->PostEventMS( &EV_PostSpawn, 0 );
		}
	}

	if ( currentState != GAMESTATE_STARTUP ) {
		gameLocal.SetGameState( currentState );
	}

	// re-enable the min spawn index
	assert( latchMinSpawnIndex == MAX_CLIENTS || gameLocal.firstFreeIndex <= latchMinSpawnIndex );
	gameLocal.minSpawnIndex = latchMinSpawnIndex;
}

void rvInstance::PopulateFromMessage( const idBitMsg& msg ) {
	bool proto69 = ( gameLocal.GetCurrentDemoProtocol() == 69 );

	if ( proto69 ) {
		numMapEntities = msg.ReadShort();
	}
	initialSpawnCount = msg.ReadShort();

	delete[] mapEntityNumbers;
	mapEntityNumbers = NULL;

	if ( proto69 ) {
		RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_LEVEL);
		mapEntityNumbers = new unsigned short[ numMapEntities ];
		RV_POP_HEAP();
		memset( mapEntityNumbers, -1, sizeof( unsigned short ) * numMapEntities );

		for ( int i = 0; i < numMapEntities; i++ ) {
			mapEntityNumbers[ i ] = msg.ReadShort();
		}
		Populate();
	} else {
		int populateIndex = msg.ReadLong();
		gameLocal.ClientSetStartingIndex( populateIndex );
		//common->Printf( "pos: set firstFreeIndex to %d\n", populateIndex );
		int checksum = msg.ReadLong();
		Populate( checksum );
	}
}

void rvInstance::Restart( void ) {
	if ( gameLocal.isMultiplayer ) {
		Populate();
	} else {
		gameLocal.SpawnMapEntities();
	}
}

void rvInstance::BuildInstanceMessage( void ) {
	// Build the client join instance msg
	mapEntityMsg.BeginWriting();
	mapEntityMsg.Init( mapEntityMsgBuf, sizeof( byte ) * MAX_GAME_MESSAGE_SIZE );
	mapEntityMsg.WriteByte( GAME_RELIABLE_MESSAGE_SET_INSTANCE );

	mapEntityMsg.WriteByte( instanceID );
	mapEntityMsg.WriteShort( initialSpawnCount );
	// we need to send that down for tourney so the index will match
	mapEntityMsg.WriteLong( gameLocal.GetStartingIndexForInstance( instanceID ) );

	LittleRevBytes( mapEntityNumbers, sizeof(unsigned short ), numMapEntities ); //DAJ
	int checksum = MD5_BlockChecksum( mapEntityNumbers, sizeof( unsigned short ) * numMapEntities );
	//common->Printf( "pop: server checksum: 0x%x\n", checksum );
	mapEntityMsg.WriteLong( checksum );
}

void rvInstance::JoinInstance( idPlayer* player ) {
	assert( player && gameLocal.isServer );

	// Transmit the instance information to the new client
	if( gameLocal.isListenServer && player == gameLocal.GetLocalPlayer() ) {
		gameLocal.mpGame.ServerSetInstance( instanceID );
	} else {
		networkSystem->ServerSendReliableMessage( player->entityNumber, mapEntityMsg );
	}
}

void rvInstance::PrintMapNumbers( void ) {
	gameLocal.Printf( "Instance: %d\n", instanceID );
	gameLocal.Printf( "Num Map Entities: %d\n", numMapEntities );

	for( int i = 0; i < numMapEntities; i++ ) {
		gameLocal.Printf( "%d\n", mapEntityNumbers[ i ] );
	}
}

/*
================
rvInstance::SetSpawnInstanceID
Sets the spawn instance ID for this instance.  On the client, only instance 0 is ever 
used, but it spawns in map entities for other instances.  spawnInstanceID is used to
spawn map entities with the correct instance number on the client.
================
*/
void rvInstance::SetSpawnInstanceID( int newInstance ) {
	assert( gameLocal.isClient );

	spawnInstanceID = newInstance;
}
