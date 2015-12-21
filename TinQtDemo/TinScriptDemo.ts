// ====================================================================================================================
// TinScriptDemo.ts
// ====================================================================================================================
Print("Welcome to the TinScript Demo!");

void ThreadTestCount(int num) {
    Print(num);
    if(num > 0)
        schedule(0, 1000, Hash("ThreadTestCount"), num - 1);
}

// ====================================================================================================================
// DefaultGame implementation
// ====================================================================================================================
void DefaultGame::OnCreate() : CScriptObject
{
    // -- this schedule is repeated, so which will reschedule itself automatically
    int self.sched_update = repeat(self, 1, hash("OnUpdate"));

    // -- declare the member, but we'll hook up the actual group in the OnInit()
    // -- this allows hooking up the member from a saved game (which also creates the group)
    object self.game_objects = create CObjectGroup("GameObjects");

    // -- we track the sim time, so OnUpdate() can calculate the deltaTime
    int self.SimTime;

    // -- schedule the OnInit(), so it can happen after a saved game is restored
    schedule(self, 1, hash("OnInit"));
}

void DefaultGame::OnInit()
{
    // -- hook up the game objects
    // -- if we don't find one, create one
    self.game_objects = FindObject("GameObjects");
    if (!IsObject(self.game_objects))
        self.game_objects = create CObjectGroup("GameObjects");
        
    // -- initialize the sim time
    self.SimTime = GetSimTime();
}

void DefaultGame::OnDestroy()
{
    // -- destroy the group, regardless of who created it
    destroy self.game_objects;
}

void DefaultGame::OnUpdate()
{
    // -- find out how much sim time has elapsed
    int curTime = GetSimTime();
    float deltaTime = (curTime - self.SimTime);
    deltaTime /= 1000.0f;
    self.SimTime = curTime;

    object cur_object = self.game_objects.First();
    while (IsObject(cur_object))
    {
        cur_object.OnUpdate(deltaTime);
        cur_object = self.game_objects.Next();
    }
}

void CreateGame()
{
    if (FindObject("CurrentGame") == 0)
    {
        create DefaultGame("CurrentGame");
    }
}

void ResetGame()
{
    // -- cancel all draw requests
    CancelDrawRequests(-1);

    object current_game = FindObject("CurrentGame");
    if (IsObject(current_game))
    {
        destroy current_game;
    }

    //  -- always start Unpaused
    SimUnpause();
}

// -- wrapper to handle events
void NotifyEvent(int keypress)
{
    object current_game = FindObject("CurrentGame");
    if (IsObject(current_game))
    {
        current_game.OnKeyPress(keypress);
    }
}

// ====================================================================================================================
// SceneObject implementation
// ====================================================================================================================
void SceneObject::OnCreate() : CScriptObject
{
    vector3f self.position;
    float self.radius;
}

void SceneObject::OnDestroy()
{
    // -- ensure we have no latent draw requests
    CancelDrawRequests(self);
}

void SceneObject::OnUpdate(float deltaTime)
{
    // -- we're using our object ID also as a draw request ID
    CancelDrawRequests(self);
    DrawCircle(self, self.position, self.radius, 0);
}

object CreateSceneObject(string name, vector3f position, float radius)
{
    // -- create the asteroid
    object scene_object = create CScriptObject(name);

    scene_object.position = position;
    if (scene_object.radius == 0.0f)
        scene_object.radius = radius;

    object current_game = FindObject("CurrentGame");
    if (IsObject(current_game))
        current_game.game_objects.AddObject(scene_object);

    // -- return the object create
    return (scene_object);
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
