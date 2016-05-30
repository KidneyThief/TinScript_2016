// ====================================================================================================================
// worms.ts
// ====================================================================================================================

// -- this game is built upon the TinScriptDemo
Include("TinScriptDemo.ts");

// -- global tunables -------------------------------------------------------------------------------------------------

object gCurrentGame;

int g_playerSize = 10;

// -- Colors ----------------------------------------------------------------------------------------------------------
int gCOLOR_BLACK = 0xff000000;
int gCOLOR_RED = 0xffff0000;
int gCOLOR_GREEN = 0xff00ff00;
int gCOLOR_YELLOW = 0xffff8f00;
int gCOLOR_BLUE = 0xff0000ff;
int gCOLOR_PURPLE = 0xffff00ff;
int gCOLOR_CYAN = 0xff00ffff;
int gCOLOR_BROWN = 0xff8fff00;

// -- Directions ------------------------------------------------------------------------------------------------------
int g_directionNone = 0;
int g_directionUp = 1;
int g_directionLeft = 2;
int g_directionRight = 3;
int g_directionDown = 4;

// ====================================================================================================================
// Player implementation
// ====================================================================================================================
void Player::OnCreate() : SceneObject
{
    int self.m_direction = g_directionNone;
    int self.m_nextDirection = g_directionNone;

    self.position;
    float self.radius;

    // -- keep track of the length
    vector3f[3072] self.m_tail;
    int self.m_currentDrawRequest = 0;
    int self.length = 0;
}

void Player::Initialize(vector3f position)
{
    self.position = position;

    // -- the radius is global
    self.radius = g_playerSize;
}

void Player::SetDirection(int direction)
{
    // -- ensure we go in a valid direction
    if (self.m_direction == g_directionUp && direction == g_directionDown)
        return;
    if (self.m_direction == g_directionDown && direction == g_directionUp)
        return;
    if (self.m_direction == g_directionLeft && direction == g_directionRight)
        return;
    if (self.m_direction == g_directionRight && direction == g_directionLeft)
        return;

    // -- set the direction
    self.m_nextDirection = direction;
}

void Player::OnUpdate(float deltaTime)
{
    // -- update the next direction for this frame
    self.m_direction = self.m_nextDirection;

    // -- update the position
    switch (self.m_direction)
    {
        case (g_directionNone):
            break;

        case (g_directionUp):
            self.position:y -= 1;
            break;

        case (g_directionDown):
            self.position:y += 1;
            break;
            
        case (g_directionLeft):
            self.position:x -= 1;
            break;
            
        case (g_directionRight):
            self.position:x += 1;
            break;
    }

    if (self.position:y < 0.0f)
    {
        self.position:y = 0.0f;
        self.OnCollision();
    }
    if (self.position:y > 47.0f)
    {
        self.position:y = 47.0f;
        self.OnCollision();
    }
    if (self.position:x < 0.0f)
    {
        self.position:x = 0.0f;
        self.OnCollision();
    }
    if (self.position:x > 63.0f)
    {
        self.position:x = 63.0f;
        self.OnCollision();
    }

    // -- if the position is occupied, game over
    if (self.m_direction != g_directionNone)
    {
        if (gCurrentGame.IsPositionOccupied(self.position))
        {
            self.OnCollision();
        }
        else
        {
            // -- see if we ate the apple
            if (gCurrentGame.FoundApplePosition(self.position))
            {
                self.length += 1;
            }

            // -- and the cancel the tail draw request
            int tail_index = (self.m_currentDrawRequest + 3072 - self.length) % 3072;
            CancelDrawRequests(tail_index);

            // -- set the tail position unoccupied
            gCurrentGame.SetPositionOccupied(self.m_tail[tail_index], false);

            // -- update the head position
            self.m_currentDrawRequest = (self.m_currentDrawRequest + 1) % 3072;

            // -- set the head position occupied
            gCurrentGame.SetPositionOccupied(self.position, true);
            self.m_tail[self.m_currentDrawRequest] = self.position;
        }
    }
    else
    {
        CancelDrawRequests(0);
        self.m_currentDrawRequest = 0;
        self.m_tail[0] = self.position;
    }

    vector3f draw_position = self.position;
    draw_position:x *= g_playerSize;
    draw_position:y *= g_playerSize;
    
    // -- draw the 4 lines creating the "triangle-ish" ship
    DrawRect(self.m_currentDrawRequest, draw_position, g_playerSize, g_playerSize, gCOLOR_BLUE);
}

void Player::OnCollision()
{
    // -- game over, at the center of the screen
    DrawText(self, '320 240 0', "G A M E   O V E R", gCOLOR_RED);
    
    SimPause();
    Print("Type StartWorms(); to continue...");
}

// ====================================================================================================================
// Worms Game implementation
// ====================================================================================================================
void WormsGame::OnCreate() : DefaultGame
{
    // -- initialize the hierarchy
    DefaultGame::OnInit();

    object self.m_playerGroup = create CObjectGroup("PlayerGroup");
    object self.m_localPlayer;

    int self.m_lastUpdateTime = GetSimTime();
    int self.m_updatePeriod = 100;

    // -- create the field
    bool[3072] self.m_arena;

    bool self.m_hasApple = false;
    vector3f self.m_apple;
    int self.m_appleTime = GetSimTime() + 4000;
}

void WormsGame::OnUpdate()
{
    // -- initialize the hierarchy
    DefaultGame::OnUpdate();

    // -- update the input
    self.UpdateKeys(self.SimTime);

    int cur_time = GetSimTime();
    int elapsed = cur_time - self.m_lastUpdateTime;
    if (elapsed >= self.m_updatePeriod)
    {
        self.m_lastUpdateTime = cur_time;

        object player = self.m_playerGroup.First();
        while (IsObject(player))
        {
            player.OnUpdate(0.0f);
            player = self.m_playerGroup.Next();
        }
    }

    // -- see if it's time to create an apple
    if (self.SimTime > self.m_appleTime && !self.m_hasApple)
    {
        self.SpawnApple();
    }

    // -- draw the apple
    if (self.m_hasApple)
    {
        CancelDrawRequests(4000);
        vector3f draw_position = self.m_apple;
        draw_position:x *= g_playerSize;
        draw_position:y *= g_playerSize;
        
        // -- draw the 4 lines creating the "triangle-ish" ship
        DrawRect(4000, draw_position, g_playerSize, g_playerSize, gCOLOR_RED);
    }
}

void WormsGame::SpawnApple()
{
    // -- find a location
    int i;
    for (i = 0; i < 100; ++i)
    {
        int rand_x = RandomInt(64);
        int rand_y = RandomInt(48);
        vector3f rand_position = StringCat(rand_x, " ", rand_y, " 0");
        if (!self.IsPositionOccupied(rand_position))
        {
            self.m_hasApple = true;
            self.m_apple = rand_position;
            break;
        }
    }

    // -- either way, update the spawn time
    self.m_appleTime = self.SimTime + 4000;
}

bool WormsGame::FoundApplePosition(vector3f position)
{
    if (self.m_hasApple && self.m_apple == position)
    {
        self.m_hasApple = false;
        self.m_updatePeriod -= 5;
        if (self.m_updatePeriod < 1)
            self.m_updatePeriod = 1;
        return (true);
    }

    // -- no apple, or not apple position
    return (false);
}

bool WormsGame::IsPositionOccupied(vector3f position)
{
    int index = position:y * 64 + position:x;
    return (self.m_arena[index]);
}

void WormsGame::SetPositionOccupied(vector3f position, bool occupied)
{
    int index = position:y * 64 + position:x;
    self.m_arena[index] = occupied;
}

void WormsGame::OnDestroy()
{
    // -- destroy all playeres
    object player = self.m_playerGroup.First();
    while (IsObject(player))
    {
        destroy player;
        player = self.m_playerGroup.Next();
    }

    // -- destroy the player set
    destroy self.m_playerGroup;
}

object WormsGame::CreatePlayer(string name, bool is_local, vector3f position)
{
    object player = create Player(name);
    player.Initialize(position);

    // -- add the player to the game objects set, so it receives OnUpdate() calls
    self.game_objects.AddObject(player);

    // -- specific organization for this game
    self.m_playerGroup.AddObject(player);
    if (is_local)
        self.m_localPlayer = player;

    return (player);
}

void WormsGame::UpdateKeys(int update_time)
{
    object local_player = self.m_localPlayer;
    if (!IsObject(local_player))
        return;

    // -- move up
    if (IsKeyPressed(KeyCode_i))
        local_player.SetDirection(g_directionUp);
    
    // -- rotate left
    if (IsKeyPressed(KeyCode_j))
        local_player.SetDirection(g_directionLeft);
    
    // -- move right
    if (IsKeyPressed(KeyCode_l))
        local_player.SetDirection(g_directionRight);
    
    // -- move down
    if (IsKeyPressed(KeyCode_k))
        local_player.SetDirection(g_directionDown);
}

void StartWorms()
{
    CancelDrawRequests(-1);
    ResetGame();
    gCurrentGame = create WormsGame("CurrentGame");

    // -- create the player
    gCurrentGame.CreatePlayer("Foo", true, "32 24 0");
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
