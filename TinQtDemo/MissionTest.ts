// ====================================================================================================================
// MissionTest.ts
// ====================================================================================================================

Include("MissionObjects.ts");

Print("Executing MissionTest.ts");

// -- globals ---------------------------------------------------------------------------------------------------------

object gCurrentGame;

void CreateMissionSim()
{
    // -- destroy the old game
    ResetGame();
    
    // -- create the new
    gCurrentGame = create CScriptObject("MissionSim");
    
    // -- create the useful mission objects
    CreateMissionTestObjects();
    
    // -- give us a player to work with
    SpawnCharacter("Player", "320 240 0");
}

// -- create a group of spawn points, one at each corner
void CreateCornerSpawnGroup()
{
    // -- create the group
    object corner_group = create SpawnGroup("CornerSpawns");
    
    if (IsObject(gCurrentGame))
        gCurrentGame.game_objects.AddObject(corner_group);
        
    // -- create the spawn points for the group
    corner_group.CreateSpawnPoint("0 0 0");
    corner_group.CreateSpawnPoint("640 0 0");
    corner_group.CreateSpawnPoint("0 480 0");
    corner_group.CreateSpawnPoint("640 480 0");
}

// -- create a group of spawn points, one at each edge
void CreateEdgeSpawnGroup()
{
    // -- create the group
    object edge_group = create SpawnGroup("EdgeSpawns");
    
    if (IsObject(gCurrentGame))
        gCurrentGame.game_objects.AddObject(edge_group);
        
    // -- create the spawn points for the group
    edge_group.CreateSpawnPoint("320 0 0");
    edge_group.CreateSpawnPoint("640 240 0");
    edge_group.CreateSpawnPoint("320 480 0");
    edge_group.CreateSpawnPoint("0 240 0");
}

// -- create mission test objects
void CreateMissionTestObjects()
{
    // -- a set of objects, shared and used by constructed mission actions
    CreateCornerSpawnGroup();
    CreateEdgeSpawnGroup();
    
    // -- create the trigger volumes
    object tv_L = CreateTriggerVolume("TV_Left", "100 100 0", "200 200 0");
    object tv_C = CreateTriggerVolume("TV_Center", "270 190 0", "370 290 0");
    object tv_R = CreateTriggerVolume("TV_Right", "440 280 0", "540 380 0");
}

// -- create a Linear Mission test
object CreateLinearMission()
{
    // -- create our linear tree
    object linear_mission_test = create MissionTreeLinear("TestLinear");
    
    // -- create the trigger volume action, and add it to the mission tree
    object exit_tv = CreatePlayerTVExitAction("PlayerExitCenter", "TV_Center");
    linear_mission_test.AddObject(exit_tv);
    
    object enter_tv = CreatePlayerTVEnterAction("PlayerEnterLeft", "TV_Left");
    linear_mission_test.AddObject(enter_tv);
    
    object enter_tv_2 = CreatePlayerTVEnterAction("PlayerEnterRight", "TV_Right");
    linear_mission_test.AddObject(enter_tv_2);
    
    // -- return the mission
    return (linear_mission_test);
}

// -- create a Parallel Mission test
object CreateParallelMission()
{
    // -- create a parallel version
    object parallel_mission_test = create MissionTreeParallel("TestParallel");
    
    // -- create the trigger volume action, and add it to the mission tree
    object exit_tv = CreatePlayerTVEnterAction("PlayerEnterCenter", "TV_Center");
    parallel_mission_test.AddObject(exit_tv);
    
    object enter_tv = CreatePlayerTVExitAction("PlayerExitLeft", "TV_Left");
    parallel_mission_test.AddObject(enter_tv);
    
    object enter_tv_2 = CreatePlayerTVExitAction("PlayerExitRight", "TV_Right");
    parallel_mission_test.AddObject(enter_tv_2);
    
    // -- return the mission
    return (parallel_mission_test);
}

void TestTriggerVolumes()
{
    ResetGame();
    gCurrentGame = create CScriptObject("MissionSim");
    
    SpawnCharacter("Player", "320 240 0");
    
    // -- create the test objects we'll need
    CreateMissionTestObjects();
    
    // -- Create the "meta" mission tree, a linear tree that owns the test missions
    object test_mission = create MissionTreeLinear("TestEncounter");
    
    // -- create the linear and parallel test missions
    object linear_test = CreateLinearMission();
    object parallel_test = CreateParallelMission();
    
    // -- now add both the linear and parallel tests to our encounter
    test_mission.AddObject(linear_test);
    test_mission.AddObject(parallel_test);
    
    // -- start the encounter
    test_mission.Begin();
}

void TestSpawnWaves()
{
    ResetGame();
    gCurrentGame = create CScriptObject("MissionSim");
    
    SpawnCharacter("Player", "320 240 0");
    
    // -- create the test objects we'll need
    CreateMissionTestObjects();
    
    // -- Create the linear mission tree
    object test_mission = create MissionTreeLinear("TestEncounter");
    
    // -- create a TV - the mission begins when the player exits the center
    object exit_tv = CreatePlayerTVExitAction("PlayerExitCenter", "TV_Center");
    test_mission.AddObject(exit_tv);
    
    // -- 5 seconds after the player exits...
    object timer_begin = CreateTimerAction("TimerBegin", 5.0f);
    test_mission.AddObject(timer_begin);
    
    // -- we spawn 3x enemies from the corner spawn points
    object spawn_first_wave = CreateSpawnWaveAction("FirstWave", "CornerSpawns", 3, 2);
    test_mission.AddObject(spawn_first_wave);
    
    object dialog_ready = CreateDialogAction("DialogWin", "Get ready for wave 2!");
    test_mission.AddObject(dialog_ready);
    
    // -- after killing two of the three, we spawn a second wave, from the edges
    object timer_second_wave = CreateTimerAction("TimerSecondWave", 3.0f);
    test_mission.AddObject(timer_second_wave);
    
    // -- 3x more enemies spawn from the edges this time
    object spawn_second_wave = CreateSpawnWaveAction("SecondWave", "EdgeSpawns", 3, 3);
    test_mission.AddObject(spawn_second_wave);
    
    // -- create success / fail dialogs
    object dialog_win = CreateDialogAction("DialogWin", "You Win!");
    spawn_second_wave.SetSuccessAction(dialog_win);
    
    object dialog_lose = CreateDialogAction("DialogLose", "You Lose!");
    spawn_first_wave.SetFailAction(dialog_lose);
    spawn_second_wave.SetFailAction(dialog_lose);
    
    // -- start the encounter
    test_mission.Begin();
}

void TestLoopWaves()
{
    ResetGame();
    gCurrentGame = create CScriptObject("MissionSim");
    
    SpawnCharacter("Player", "320 240 0");
    
    // -- create the test objects we'll need
    CreateMissionTestObjects();
    
    // -- Create the linear mission tree
    object test_mission = create MissionTreeLinear("TestEncounter");
    
    // -- create a TV - the mission begins when the player exits the center
    object exit_tv = CreatePlayerTVExitAction("MissionStart", "TV_Center");
    test_mission.AddObject(exit_tv);
    
    // -- we want a spawn action, and a timer action to run in parallel
    object spawn_loop_tree = create MissionTreeParallel("SpawnLoopTree");
    test_mission.AddObject(spawn_loop_tree);
    
    // -- to this tree, we add a linear tree, containing a spawn wave action, followed by a reset action
    // -- so if we successfully defeat the wave, we defeat the game
    object spawn_wave_tree = create MissionTreeLinear("SpawnWaveTree");
    spawn_loop_tree.AddObject(spawn_wave_tree);
    
    // -- create the spawn action
    object spawn_first_wave = CreateSpawnWaveAction("FirstWave", "CornerSpawns", 3, 3);
    spawn_wave_tree.AddObject(spawn_first_wave);
    
    // -- create success / fail dialogs
    object dialog_win = CreateDialogAction("DialogWin", "You Win!");
    spawn_first_wave.SetSuccessAction(dialog_win);
    
    object dialog_lose = CreateDialogAction("DialogLose", "You Lose!");
    spawn_first_wave.SetFailAction(dialog_lose);
    
    // -- create the reset action
    object reset_action = CreateResetAction("ResetWave");
    spawn_wave_tree.AddObject(reset_action);
    reset_action.AddAction(test_mission);

    // -- and a timer running in parallel
    object timer_spawn_wave = CreateTimerAction("SpawnWaveTimer", 30.0f);
    spawn_loop_tree.AddObject(timer_spawn_wave);
    
    // -- upon the timer successfully counting down, we want to restart the SpawnLoopTree
    // -- which will restart this timer, as well as respawn more enemies as the spawn wave is restarted
    timer_spawn_wave.SetSuccessAction(spawn_loop_tree);
    
    // -- start the mission
    test_mission.Begin();
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
//
// MISSION DEMO NOTES
//
// 1.  CreateMissionSim();   <---  base class to handle an update group, as well as sets to manage bullets, characters...
//
// 2.  object tv = CreateTriggerVolume("tv", "100 100 0", "200 200 0"); <--- functional objects
//     tv.AddNotify(tv);
//
// 4.  object tv_enter = CreatePlayerTVEnterAction("tv_enter", "TV_Left");
//     tv_enter.Begin();  <---  Begin / Success demo of an action
//     tv_enter.Begin();  <---  Independent and repeatedly functional
//
// 5.  object tv_exit = CreatePlayerTVExitAction("tv_exit", "TV_Right");
//     tv_exit.Begin();  <---  Independent and repeatedly functional
//
// 6.  tv_enter.Begin();  tv_exit.Begin();  <-- simultaneous
//
// 7.  object linear_tree = create MissionTreeLinear("linear_tree");
//     linear_tree.AddObject(tv_enter);
//     linear_tree.AddObject(tv_exit);
//     linear_tree.Begin();   <--- woot!
//
// 8.  I'm sure you can figure out how a parallel tree worked
//     object test_parallel = CreateParallelMission();
//     test_parallel.Begin();
//
// 9.  TestTriggerVolumes();   <---  3x linear TV actions, followed by the parallel tree, which contains 3x more actions in parallel.
//
// 10.  *  Actions have 4 status:  NONE, PENDING, SUCCESS, FAIL
//      *  Actions can be groups (trees are an example)
//      *  Actions have a "OnSuccess" action object
//      *  Actions have a "OnFail" action object
//
//  11. object timer_action = CreateTimerAction("timer_action", 20.0f);
//      object spawn_wave = CreateSpawnWaveAction("spawn_wave", "EdgeSpawns", 3, 2);
//      timer_action.SetSuccessAction(spawn_wave);
//      timer_action.Begin();
// 
//  12  TestSpawnWaves();  <--  TV, timer, spawn wave 1, on success, timer, spawn wave 2 -> Dialog Win / Dialog Fail
//
//
//  13. TestLoopWaves();  <--  TV, Parallel ->  Linear -> Spawn Wave -> Dialog Win -> Reset  (the entire tree)
//                                  ^  |
//                                  |  '----->  Timer  ---,
//                                  |                     |
//                                  '---------------------'
//


