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

    // -- if we're the challenger, notify the host
    if (!gCurrentGame.m_isHost)
        SocketCommand("NotifyPlayerMove", direction);
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
                self.length += 3;
            }
        }
    }

    // -- update the player's position
    self.NotifyPosition();
}

void Player::NotifyPosition()
{
    bool is_local_player = (self == gCurrentGame.m_localPlayer);

    int draw_request_offset = 0;
    if (!is_local_player)
        draw_request_offset = 4000;

    int player_color =gCOLOR_BLUE;
    if (!is_local_player)
        player_color = gCOLOR_GREEN;

    // -- if the position is occupied, game over
    if (!gCurrentGame.m_isHost || self.m_direction != g_directionNone)
    {
        // -- and the cancel the tail draw request
        int tail_index = (self.m_currentDrawRequest + 3072 - self.length) % 3072;
        CancelDrawRequests(tail_index + draw_request_offset);

        // -- set the tail position unoccupied
        gCurrentGame.SetPositionOccupied(self.m_tail[tail_index], false);

        // -- update the head position
        self.m_currentDrawRequest = (self.m_currentDrawRequest + 1) % 3072;

        // -- set the head position occupied
        gCurrentGame.SetPositionOccupied(self.position, true);
        self.m_tail[self.m_currentDrawRequest] = self.position;
    }
    else
    {
        CancelDrawRequests(0 + draw_request_offset);
        self.m_currentDrawRequest = 0;
        self.m_tail[0] = self.position;
    }

    vector3f draw_position = self.position;
    draw_position:x *= g_playerSize;
    draw_position:y *= g_playerSize;
    
    // -- draw the 4 lines creating the "triangle-ish" ship
    DrawRect(self.m_currentDrawRequest + draw_request_offset, draw_position, g_playerSize, g_playerSize, player_color);

    // -- notify the client
    if (gCurrentGame.m_isHost)
        SocketCommand("NotifyPlayerUpdate", is_local_player, self.position, self.length);
}

void Player::OnCollision()
{
    // -- game over, at the center of the screen
    bool is_local_player = (self == gCurrentGame.m_localPlayer);
    if (is_local_player)
        DrawText(8000, '320 240 0', "Y O U   L O S E", gCOLOR_RED);
    else
        DrawText(8000, '320 240 0', "Y O U   W I N", gCOLOR_RED);

    // -- notify the client
    SocketCommand("NotifyGameOver", is_local_player);
    
    SimPause();
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
    int self.m_updatePeriod = 200;

    // -- create the field
    bool[3072] self.m_arena;

    bool self.m_hasApple = false;
    vector3f self.m_apple;
    int self.m_appleTime = GetSimTime() + 4000;

    // -- issue the countdown
    int self.m_updateTime = GetSimTime();
    int self.m_gameCountdown = 5000;
    bool self.m_gameStarted = false;
    bool self.m_isHost = false;
    int self.m_clientUpdateTime = 0;
}

void WormsGame::OnUpdate()
{
    // -- initialize the hierarchy
    DefaultGame::OnUpdate();

    int deltaTime = GetSimTime() - self.m_updateTime;
    self.m_updateTime = GetSimTime();

    // -- if we haven't started yet, we're done
    if (!self.m_gameStarted)
    {
        return;
    }

    // -- perform the countdown
    if (self.m_gameCountdown > 0)
    {
        self.m_gameCountdown -= deltaTime;
        int seconds = (self.m_gameCountdown / 1000) + 1;

        // -- game over, at the center of the screen
        CancelDrawRequests(8000);
        DrawText(8000, '320 240 0', seconds, gCOLOR_RED);

        // -- start both players moving
        if (self.m_gameCountdown <= 0)
        {
            object player = self.m_playerGroup.First();
            while (IsObject(player))
            {
                player.m_nextDirection = g_directionUp;
                player = self.m_playerGroup.Next();
            }
        }
    }

    // -- update the input
    self.UpdateKeys(self.SimTime);

    // -- only the host updates the players
    if (gCurrentGame.m_isHost)
    {
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
    }

    if (self.m_gameCountdown > 0)
        return;

    // -- see if it's time to create an apple
    if (self.m_isHost && self.SimTime > self.m_appleTime && !self.m_hasApple)
    {
        self.SpawnApple();
    }

    // -- draw the apple
    CancelDrawRequests(8000);
    if (self.m_hasApple)
    {
        vector3f draw_position = self.m_apple;
        draw_position:x *= g_playerSize;
        draw_position:y *= g_playerSize;
        
        // -- draw the 4 lines creating the "triangle-ish" ship
        DrawRect(8000, draw_position, g_playerSize, g_playerSize, gCOLOR_RED);
    }

    // -- if we're the host, notify the client
    if (self.m_isHost)
        SocketCommand("NotifyApple", self.m_hasApple, self.m_apple);
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
        if (self.m_updatePeriod < 10)
            self.m_updatePeriod = 10;

        // -- update the spawn time
        self.m_appleTime = self.SimTime + 4000;

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

    // -- specific organization for this game
    self.m_playerGroup.AddObject(player);
    if (is_local)
        self.m_localPlayer = player;

    return (player);
}

void WormsGame::UpdateKeys(int update_time)
{
    if (self.m_gameCountdown > 0)
        return;

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

void StartWorms(string player_name)
{
    CancelDrawRequests(-1);
    ResetGame();
    gCurrentGame = create WormsGame("CurrentGame");
    gCurrentGame.m_isHost = true;

    // -- create the player
    gCurrentGame.CreatePlayer(player_name, true, "22 24 0");

    // -- this will be the host
    SocketListen();
}

void ChallengeWorms(string player_name, string ip_address)
{
    CancelDrawRequests(-1);
    ResetGame();
    gCurrentGame = create WormsGame("CurrentGame");
    gCurrentGame.m_isHost = false;

    // -- create the player
    gCurrentGame.CreatePlayer(player_name, true, "42 24 0");

    // -- the challenger is player two
    SocketConnect(ip_address);
    SocketCommand("NotifyChallenge", player_name);
}

void NotifyChallenge(string challenger_name)
{
    // -- create the player
    gCurrentGame.CreatePlayer(challenger_name, false, "42 24 0");
    SocketCommand("AcceptChallenge", gCurrentGame.m_localPlayer.GetObjectName());

    // -- set the bool to start the game
    gCurrentGame.m_gameStarted = true;
}

void AcceptChallenge(string challenger_name)
{
    // -- create the player
    gCurrentGame.CreatePlayer(challenger_name, false, "24 24 0");

    // -- set the bool to notify that the game has started
    gCurrentGame.m_gameStarted = true;
}

void NotifyPlayerMove(int direction)
{
    object challenger = gCurrentGame.m_playerGroup.GetObjectByIndex(1);
    if (IsObject(challenger))
    {
        challenger.SetDirection(direction);
    }
}

void NotifyPlayerUpdate(bool is_local_player, vector3f position, int length)
{
    // -- find the player
    int player_index = 0;
    if (!is_local_player)
        player_index = 1;
    object player = gCurrentGame.m_playerGroup.GetObjectByIndex(player_index);
    player.position = position;
    player.length = length;
    player.NotifyPosition();
}

void NotifyApple(bool has_apple, vector3f apple_position)
{
    gCurrentGame.m_hasApple = has_apple;
    gCurrentGame.m_apple = apple_position;
}

void NotifyGameOver(bool you_win)
{
    if (!you_win)
        DrawText(8000, '320 240 0', "Y O U   L O S E", gCOLOR_RED);
    else
        DrawText(8000, '320 240 0', "Y O U   W I N", gCOLOR_RED);

    SimPause();
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
