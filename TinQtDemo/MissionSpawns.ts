// ====================================================================================================================
// MissionSpawns.ts
// ====================================================================================================================
Print("Executing MissionSpawns.ts");

// -- globals ---------------------------------------------------------------------------------------------------------

object gCurrentGame;

// ====================================================================================================================
// SpawnPoint implementation
// ====================================================================================================================
void SpawnPoint::OnCreate()
{
    vector3f self.position;
}

void SpawnPoint::Initialize(vector3f pos)
{
    self.position = pos;
}

// ====================================================================================================================
// SpawnGroup implementation
// ====================================================================================================================
LinkNamespaces("SpawnGroup", "CObjectGroup");
void SpawnGroup::OnCreate()
{
    object self.temp_set = create CObjectSet("TempSet");
}

void SpawnGroup::OnDestroy()
{
    destroy self.temp_set;
}

void SpawnGroup::OnUpdate(float delta_time)
{
}

bool SpawnGroup::IsOccupied(object test_spawn)
{
    // -- sanity check
    if (!IsObject(test_spawn) || !IsObject(gCurrentGame) || !IsObject(gCurrentGame.character_set))
        return (false);
        
    // -- loop through the game's character set, see if anyone is within range
    vector3f spawn_pos = test_spawn.position;
    object character = gCurrentGame.character_set.First();
    while (IsObject(character))
    {
        // -- see if this character's position is too close
        if (V3fLength(spawn_pos - character.position) < (character.radius * 2.0f))
        {
            // -- occupied
            return (true);
        }
        character = gCurrentGame.character_set.Next();
    }
    
    // -- not occupied
    return (false);
}

object SpawnGroup::PickRandom()
{
    // -- find a spawn position that isn't already occupied
    object spawn_point = self.First();
    while (IsObject(spawn_point))
    {
        self.temp_set.AddObject(spawn_point);
        spawn_point = self.Next();
    }
    
    // -- pick a random number between
    while (self.temp_set.Used() > 0)
    {
        int count = self.temp_set.Used();
        int index = RandomInt(count);
        object test_spawn = self.temp_set.GetObjectByIndex(index);
        self.temp_set.RemoveObject(test_spawn);
        // $$$TZA !self.IsOccupied() isn't working
        if (self.IsOccupied(test_spawn) == false)
        {
            self.temp_set.RemoveAll();
            return (test_spawn);
        }
    }
    
    // -- apparently we were unable to find an unoccupied spawn position
    return (0);
}

object SpawnGroup::CreateSpawnPoint(vector3f position)
{
    object spawn_point = create CScriptObject("SpawnPoint");
    spawn_point.Initialize(position);
    self.AddObject(spawn_point);
}

// ====================================================================================================================
// SpawnWave implementation
// ====================================================================================================================
LinkNamespaces("SpawnWave", "CScriptObject");
void SpawnWave::OnCreate()
{
    // -- create a set of minions that spawned by this wave
    object self.minion_set = create CObjectSet("Minions");
    
    // -- reference to the parent mission objective, for whom the wave was created
    object self.parent_action;
    
    // -- reference to the set of spawn points used by this wave
    object self.spawn_set;
}

void SpawnWave::Initialize(object parent_action, object spawn_set)
{
    // -- initialize the spawn wave with their parent objective, and the set of points they're to use
    self.parent_action = parent_action;
    self.spawn_set = spawn_set;
}

void SpawnWave::OnDestroy()
{
    // -- only the minion_set was created by this object, and must be cleaned up
    destroy self.minion_set;
}

void SpawnWave::NotifyOnKilled(object dead_minion)
{
    // -- remove the object from the set
    self.minion_set.RemoveObject(dead_minion);
    
    // -- notify the mission objective.
    if (IsObject(self.parent_action))
    {
        if (ObjectHasMethod(self.parent_action, "NotifyOnKilled"))
            self.parent_action.NotifyOnKilled(dead_minion);
        
        // -- and to demonstrate for fun, a made up event like "last man standing"
        if (self.minion_set.Used() == 1 && ObjectHasMethod(self.parent_action, "NotifyLastManStanding"))
            self.parent_action.NotifyLastManStanding();
    }
}

void SpawnWave::Spawn(int count)
{
    // -- validate the object
    if (!IsObject(self.spawn_set))
        return;
        
    int i;
    for (i = 0; i < count; i += 1)
    {
        object spawn_point = self.spawn_set.PickRandom();
        if (IsObject(spawn_point))
        {
            object enemy = SpawnCharacter("Enemy", spawn_point.position);
            self.minion_set.AddObject(enemy);
            enemy.AddNotifyOnKilled(self);
        }
    }
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
