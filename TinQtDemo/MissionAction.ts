// ====================================================================================================================
// MissionTree.ts
// ====================================================================================================================
Print("Executing MissionAction.ts");

// -- globals ---------------------------------------------------------------------------------------------------------

object gCurrentGame;

// ====================================================================================================================
// PlayerEnterTriggerAction implementation - sets "success" when the player enters the trigger volume
// ====================================================================================================================
void PlayerEnterTriggerAction::OnCreate() : ActionTriggerVolume
{
    // -- construct the base class
}

void PlayerEnterTriggerAction::NotifyOnEnter(object character)
{
    if (character.team == TEAM_ALLY)
    {
        self.Complete(true);
    }
}

// ====================================================================================================================
// PlayerExitTriggerAction implementation - sets "success" when the player enters the trigger volume
// ====================================================================================================================
void PlayerExitTriggerAction::OnCreate() : ActionTriggerVolume
{
    // -- construct the base class
}

void PlayerExitTriggerAction::NotifyOnExit(object character)
{
    if (character.team == TEAM_ALLY)
    {
        self.Complete(true);
    }
}

// ====================================================================================================================
// CountdownTimerAction implementation - sets "success" when the timer has elapsed
// ====================================================================================================================
void CountdownTimerAction::OnCreate() : MissionAction
{
    // -- construct the base class
    MissionAction::OnCreate();
    
    // -- timer is in seconds
    float self.duration = 0.0f;
    float self.timer = 0.0f;
}

void CountdownTimerAction::Initialize(float duration)
{
    // -- initialize the time
    self.duration = duration;
    self.timer = duration;
}

void CountdownTimerAction::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- initialize the timer and set the status
    self.timer = self.duration;
    self.status = STATUS_PENDING;
}

void CountdownTimerAction::OnUpdate(float delta_time)
{
    // -- if our status isn't "begin", we're done
    if (self.status != STATUS_PENDING)
        return;
        
    // -- cancel our draw requests
    CancelDrawRequests(self);
    
    // -- draw the remaining time in the top left corner
    DrawText(self, "600 10 0", self.timer, gCOLOR_RED);
    
    // -- decrement the timer - if we hit the end, set the status
    self.timer -= delta_time;
    if (self.timer <= 0.0f)
    {
        self.Complete(true);
        CancelDrawRequests(self);
    }
}

// ====================================================================================================================
// SpawnWaveAction implementation
// ====================================================================================================================
// -- spawns a wave of enemies, returns success if a given percentage is killed, fail if the player is killed
void SpawnWaveAction::OnCreate() : MissionAction
{
    // -- construct the base class
    MissionAction::OnCreate();
    
    // -- we need a spawn wave object, a spawn count for how many NPCs to spawn,and a completion number
    object self.spawn_wave;
    int self.spawn_count;
    int self.completion_count;
    int self.need_to_kill;
}

void SpawnWaveAction::OnDestroy()
{
    // -- first we reset
    self.Reset();
    
    // -- the action owns the spawn wave object
    if (IsObject(self.spawn_wave))
        destroy self.spawn_wave;
}

void SpawnWaveAction::Initialize(object spawn_wave, int spawn_count, float completion_count)
{
    self.spawn_wave = spawn_wave;
    self.spawn_count = spawn_count;
    self.completion_count = completion_count;
    self.need_to_kill = 0;
    
    // -- hook the spawn wave up to us as the parent action
    if (IsObject(spawn_wave))
        spawn_wave.parent_action = self;
}

void SpawnWaveAction::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- set the status
    self.status = STATUS_PENDING;
    
    // -- spawn the NPCs
    if (IsObject(self.spawn_wave))
    {
        self.spawn_wave.Spawn(self.spawn_count);
        
        // -- ensure the need_to_kill number isn't higher than the number successfully spawned
        self.need_to_kill += self.completion_count;
        int number_spawned = self.spawn_wave.minion_set.Used();
        if (self.need_to_kill > number_spawned)
            self.need_to_kill = number_spawned;
    }
    
    // -- add ourself to the player's death list
    if (IsObject(gCurrentGame.player))
        gCurrentGame.player.AddNotifyOnKilled(self);
}

void SpawnWaveAction::NotifyOnKilled(object victim)
{
    // -- if this was the player, we complete with a "fail"
    if (victim == gCurrentGame.player)
    {
        self.Complete(false);
        return;
    }
    
    // -- decrement the number needed to complete the action
    self.need_to_kill -= 1;
    if (self.need_to_kill <= 0)
        self.Complete(true);
}

void SpawnWaveAction::Reset()
{
    // -- remove ourself from the player's notify kill
    if (IsObject(gCurrentGame.player))
        gCurrentGame.player.RemoveNotifyOnKilled(self);
    
    // -- call the base class
    MissionAction::Reset();
}

void SpawnWaveAction::Complete(bool success)
{
    // -- remove ourself from the player's notify kill
    if (IsObject(gCurrentGame.player))
        gCurrentGame.player.RemoveNotifyOnKilled(self);
    
    // -- call the base class
    MissionAction::Complete(success);
}

// ====================================================================================================================
// ResetAction implementation
// ====================================================================================================================
// -- contains a set of actions, on which to call Reset(), if this action is ever started
void ResetAction::OnCreate() : MissionAction
{
    // -- construct the base class
    MissionAction::OnCreate();
    
    // -- create our set
    object self.reset_list = create CObjectSet("ResetList");
}

void ResetAction::OnDestroy()
{
    destroy self.reset_list;
}

void ResetAction::AddAction(object action)
{
    self.reset_list.AddObject(action);
}

void ResetAction::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- loop through, and call begin on all 
    object reset_action = self.reset_list.First();
    while (reset_action)
    {
        reset_action.Reset();
        reset_action = self.reset_list.Next();
    }
    
    // -- we're complete (successfully)
    self.Complete(true);
}

// ====================================================================================================================
// DialogAction implementation
// ====================================================================================================================
// -- contains a set of actions, on which to call Reset(), if this action is ever started
void DialogAction::OnCreate() : MissionAction
{
    // -- construct the base class
    MissionAction::OnCreate();
    
    // -- create our set
    string self.message;
}

void DialogAction::Initialize(string message)
{
    self.message = message;
}

void DialogAction::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- pause the game
    SimPause();
    
    // -- register ourself with the current game
    gCurrentGame.SetNotifyDialogOK(self);
    
    // -- display the message
    DrawText(self, "300 240 0", self.message, gCOLOR_BLUE);
}

void DialogAction::OnDialogOK()
{
    // -- we'd only receive this if we asked for it
    // -- clear ourself from receiving another
    gCurrentGame.SetNotifyDialogOK(0);
    
    // -- clear the text
    CancelDrawRequests(self);
    
    // -- unpause the game
    SimUnpause();
    
    // -- and we're done
    self.Complete(true);
}

// ====================================================================================================================
// Mission interface
// ====================================================================================================================
object CreateTriggerVolume(string volume_name, vector3f top_left, vector3f bottom_right)
{
    // -- create the trigger volume, and add it to the 
    object new_tv = create TriggerVolume(volume_name);
    new_tv.Initialize(top_left, bottom_right);
    
    // -- ensure it gets added to the game's update group
    gCurrentGame.game_objects.AddObject(new_tv);
    
    // -- return the object
    return (new_tv);
}

object CreatePlayerTVEnterAction(string action_name, string tv_name)
{
    object new_action_tv = create PlayerEnterTriggerAction(action_name);
    new_action_tv.Initialize(FindObject(tv_name));
    
    // -- return the action
    return (new_action_tv);
}

object CreatePlayerTVExitAction(string action_name, string tv_name)
{
    object new_action_tv = create PlayerExitTriggerAction(action_name);
    new_action_tv.Initialize(FindObject(tv_name));
    
    // -- return the action
    return (new_action_tv);
}

object CreateTimerAction(string action_name, int duration)
{
    object timer_action = create CountdownTimerAction(action_name);
    timer_action.Initialize(duration);
    
    // -- return the object
    return (timer_action);
}

object CreateSpawnWave(string wave_name, string spawn_set_name)
{
    object spawn_wave = create SpawnWave(wave_name);
    spawn_wave.Initialize(0, FindObject(spawn_set_name));
    
    // -- return the result
    return (spawn_wave);
}

object CreateSpawnWaveAction(string action_name, string spawn_set_name, int spawn_count, int completion_count)
{
    // -- first create the spawn wave - note, the spawn action owns the spawn wave
    object spawn_wave = CreateSpawnWave(StringCat(action_name, "Wave"), spawn_set_name);
    
    // -- now create the action
    object spawn_wave_action = create SpawnWaveAction(action_name);
    
    // -- initialize
    spawn_wave_action.Initialize(spawn_wave, spawn_count, completion_count);
    
    // -- return the object
    return (spawn_wave_action);
}

object CreateResetAction(string action_name)
{
    object reset_action = create ResetAction(action_name);
    return (reset_action);
}

object CreateDialogAction(string action_name, string message)
{
    if (StringCmp(message, "") == false)
        message = "Press 'q'";
    object dialog_action = create DialogAction(action_name);
    dialog_action.Initialize(message);
    
    // -- return the object
    return (dialog_action);
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
