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

    int self.m_score = 0;
    bool self.m_hasCollided = false;

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
    self.m_hasCollided = false;

    // -- the radius is global
    self.radius = g_playerSize;

    // -- move initialization
    self.m_direction = g_directionNone;
    self.m_nextDirection = g_directionNone;

    // -- tail initialization
    self.m_currentDrawRequest = 0;
    self.length = 0;
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
        SocketCommand("Host_NotifyPlayerMove", direction);
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

            // -- if we're updating second - ensure the first player isn't at the same spot
            object host_player = gCurrentGame.m_localPlayer;
            if (self != host_player && self.position == host_player.position)
            {
                host_player.OnCollision();
            }
        }

        // -- see if we ate the apple
        else if (gCurrentGame.FoundApplePosition(self.position))
        {
            self.length += 3;
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
    {
        int current_time = GetSimTime();
        if (gCurrentGame.m_requireResponseID > current_time)
            ++gCurrentGame.m_requireResponseID;
        else
            gCurrentGame.m_requireResponseID = current_time;
        SocketCommand("Client_NotifyPlayerUpdate", gCurrentGame.m_requireResponseID, is_local_player, self.position, self.length);
    }
}

void Player::OnCollision()
{
    // -- notify both players
    self.m_hasCollided = true;
}

// ====================================================================================================================
// Apple implementation
// ====================================================================================================================
void Apple::OnCreate() : CScriptObject
{
    vector3f self.m_position;
    bool self.m_spawned = false;

    // -- spawn at a random time
    self.Respawn();
}

void Apple::Spawn()
{
    // -- find a location
    int i;
    for (i = 0; i < 100; ++i)
    {
        int rand_x = RandomInt(64);
        int rand_y = RandomInt(48);
        vector3f rand_position = StringCat(rand_x, " ", rand_y, " 0");
        if (!gCurrentGame.IsPositionOccupied(rand_position))
        {
            self.m_spawned = true;
            self.m_position = rand_position;
            break;
        }
    }

    if (!self.m_spawned)
        self.Respawn();
}

void Apple::Respawn()
{
    // -- set the flag, and respawn at a random time
    self.m_spawned = false;
    int rand_time = 1000 + RandomInt(3000);
    schedule(self, rand_time, hash("Spawn"));
}

// ====================================================================================================================
// Worms Game implementation
// ====================================================================================================================
void WormsGame::OnCreate() : DefaultGame
{
    // -- initialize the hierarchy
    DefaultGame::OnInit();

    // -- need to determine if we've lost a connection
    bool self.m_isConnected = false;

    object self.m_playerGroup = create CObjectGroup("PlayerGroup");
    object self.m_localPlayer;

    int self.m_lastUpdateTime = GetSimTime();
    int self.m_updatePeriod = 100;

    // -- create the field
    bool[3072] self.m_arena;

    // -- the set of apples
    object self.m_appleGroup = create CObjectGroup("AppleGroup");
    int self.m_appleCount = 0;
    int self.m_clientAppleCount = 0;
    vector3f[16] self.m_clientApples;

    // -- issue the countdown
    int self.m_updateTime = GetSimTime();
    int self.m_gameCountdown = 5000;
    bool self.m_gameStarted = false;
    bool self.m_gameRematch = false;
    bool self.m_isHost = false;

    int self.m_requireResponseID = 0;
}

void WormsGame::Restart()
{
    // -- clear all draw requests
    CancelDrawRequests(-1);

    // -- reset the game countdown and state bools
    self.m_gameCountdown = 5000;
    self.m_gameStarted = false;
    self.m_gameRematch = false;

    // -- perform the reset needed by the host
    if (self.m_isHost)
    {
        // -- reset the arena
        int i;
        for (i = 0; i < 3072; ++i)
            self.m_arena[i] = false;

        // -- clear the apples group
        destroy self.m_appleGroup;
        object self.m_appleGroup = create CObjectGroup("AppleGroup");
        self.m_clientAppleCount = 0;

        // -- initialize the player positions
        object local_player = self.m_playerGroup.GetObjectByIndex(0);
        object challenger = self.m_playerGroup.GetObjectByIndex(1);
        if (RandomInt(100) < 50)
        {
            local_player.Initialize("22 24 0");
            if (IsObject(challenger))
                challenger.Initialize("42 24 0");
        }
        else
        {
            local_player.Initialize("42 24 0");
            if (IsObject(challenger))
                challenger.Initialize("22 24 0");
        }

        // -- update the player positions
        if (IsObject(challenger))
        {
            int current_time = GetSimTime();
            gCurrentGame.m_requireResponseID = current_time + 1;
            SocketCommand("Client_NotifyPlayerUpdate", current_time, false, local_player.position, 1);
            SocketCommand("Client_NotifyPlayerUpdate", current_time + 1, true, challenger.position, 1);

            // -- notify the challenger we're waiting for a re-match
            SocketCommand("Client_NotifyRematchRequest");
        }
    }
}

void WormsGame::OnDestroy()
{
    destroy self.m_playerGroup;
    destroy self.m_appleGroup;
}

void WormsGame::OnUpdate()
{
    // -- initialize the hierarchy
    DefaultGame::OnUpdate();

    // -- if we've lost the connection...
    if (self.m_isConnected && !SocketIsConnected())
    {
        // -- the game is no longer started
        gCurrentGame.m_gameStarted = false;

        // -- print the message - note, OnUpdate() will still get called every frame
        CancelDrawRequests(9000);
        DrawText(9000, '320 400 0', "Your connection has been lost...", gCOLOR_BLUE);
        return;
    }

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

            // -- spawn the apples
            int i;
            for (i = 0; i < self.m_appleCount; ++i)
                self.m_appleGroup.AddObject(create Apple());
        }
    }

    // -- only the host updates the players
    if (self.m_isHost)
    {
        int cur_time = GetSimTime();
        int elapsed = cur_time - self.m_lastUpdateTime;
        bool has_response = self.m_requireResponseID == 0 || self.m_playerGroup.Used() < 2;
        if (elapsed >= self.m_updatePeriod && has_response)
        {
            self.m_lastUpdateTime = cur_time;

            object player = self.m_playerGroup.First();
            while (IsObject(player))
            {
                player.OnUpdate(0.0f);
                player = self.m_playerGroup.Next();
            }

            // -- if either player has collided, the game is over
            int collision_count = 0;
            object challenger = self.m_playerGroup.GetObjectByIndex(1);
            if (self.m_localPlayer.m_hasCollided)
                ++collision_count;
            if (IsObject(challenger) && challenger.m_hasCollided)
                ++collision_count;


            bool host_wins;
            if (collision_count == 1)
            {
                host_wins = !gCurrentGame.m_localPlayer.m_hasCollided;
                SocketCommand("NotifyGameOver", !host_wins, false);
                NotifyGameOver(host_wins, false);
            }
            else if (collision_count == 2)
            {
                SocketCommand("NotifyGameOver", false, true);
                NotifyGameOver(false, true);
            }

            // -- notify the client of the updated apple positions
            SocketCommand("Client_NotifyClearApples");
            object apple = self.m_appleGroup.First();
            while (IsObject(apple))
            {
                if (apple.m_spawned)
                    SocketCommand("Client_NotifyApple", apple.m_position);
                apple = self.m_appleGroup.Next();
            }
        }
        else if (self.m_requireResponseID > 0)
        {
            int elapsed = GetSimTime() - self.m_requireResponseID;
            if (elapsed > 5000)
            {
                Print("ERROR - no response to update packet from the client!");
                self.m_requireResponseID = 0;
            }
        }
    }

    // -- if we haven't actually started, no updates
    if (self.m_gameCountdown > 0)
        return;

    // -- draw the apples
    CancelDrawRequests(8000);

    // -- if we're the host, draw the apples based on the actual group
    if (self.m_isHost)
    {
        object apple = self.m_appleGroup.First();
        while (IsObject(apple))
        {
            if (apple.m_spawned)
            {
                vector3f draw_position = apple.m_position;
                draw_position:x *= g_playerSize;
                draw_position:y *= g_playerSize;
                
                // -- draw the apple in red
                DrawRect(8000, draw_position, g_playerSize, g_playerSize, gCOLOR_RED);
            }
            apple = self.m_appleGroup.Next();
        }
    }
    else
    {
        int i;
        for (i = 0; i < self.m_clientAppleCount; ++i)
        {
            vector3f draw_position = gCurrentGame.m_clientApples[i];
            draw_position:x *= g_playerSize;
            draw_position:y *= g_playerSize;
            
            // -- draw the 4 lines creating the "triangle-ish" ship
            DrawRect(8000, draw_position, g_playerSize, g_playerSize, gCOLOR_RED);
        }
    }
}

bool WormsGame::FoundApplePosition(vector3f position)
{
    object apple = self.m_appleGroup.First();
    while (IsObject(apple))
    {
        if (apple.m_spawned && apple.m_position == position)
        {
            apple.Respawn();
            self.m_updatePeriod -= 5;
            return (true);
        }
        apple = self.m_appleGroup.Next();
    }

    // -- no apples eaten
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

void WormsGame::OnKeyEvent(int key_code, bool pressed)
{
    if (self.m_gameCountdown > 0)
        return;

    object local_player = self.m_localPlayer;
    if (!IsObject(local_player))
        return;

    // -- only handle key presses
    if (!pressed)
        return;
    
    // -- move up
    if (key_code == KeyCode_i)
        local_player.SetDirection(g_directionUp);
    
    // -- rotate left
    if (key_code == KeyCode_j)
        local_player.SetDirection(g_directionLeft);
    
    // -- move right
    if (key_code == KeyCode_l)
        local_player.SetDirection(g_directionRight);
    
    // -- move down
    if (key_code == KeyCode_k)
        local_player.SetDirection(g_directionDown);

    // -- enter
    if (key_code == KeyCode_enter)
        self.DialogPressEnter();
}

void WormsGame::DialogPlayAgain()
{
    // -- set the bool
    self.m_gameRematch = true;

    // -- if we're the host, wait for a "press to issue rematch"
    DrawText(9000, '320 400 0', "Press <return> to play again!", gCOLOR_BLUE);
}

void WormsGame::DialogPressEnter()
{
    if (!self.m_gameRematch)
        return;

    // -- we must have pressed enter
    gCurrentGame.Restart();

    // -- if we're not the host, then we're accepting the challenge
    if (self.m_isHost)
        DrawText(8000, "320 240 0", "Waiting for challenger...", gCOLOR_BLUE);
    else
        SocketCommand("Host_NotifyChallenge");
}

// -- network related functions ----------------------------------------------------------------------------------

void StartWorms(string player_name, int apple_count)
{
    CancelDrawRequests(-1);
    ResetGame();
    gCurrentGame = create WormsGame("CurrentGame");
    gCurrentGame.m_isHost = true;

    // -- set the apple count, to be created when the game is actually started
    if (apple_count < 1)
        apple_count = 1;
    else if (apple_count > 16)
        apple_count = 16;
    gCurrentGame.m_appleCount = apple_count;

    // -- create the player
    gCurrentGame.CreatePlayer(player_name, true, "22 24 0");

    DrawText(8000, "320 240 0", "Waiting for challenger...", gCOLOR_BLUE);

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
    if (!SocketIsConnected())
        SocketConnect(ip_address);

    SocketCommand("Host_NotifyChallenge", player_name);
}

void Host_NotifyChallenge(string challenger_name)
{
    // -- create the player
    if (gCurrentGame.m_playerGroup.Used() < 2)
        gCurrentGame.CreatePlayer(challenger_name, false, "42 24 0");

    // -- set the connected bool
    gCurrentGame.m_isConnected = true;

    // -- send the handshake
    SocketCommand("Client_AcceptChallenge", gCurrentGame.m_localPlayer.GetObjectName());

    // -- set the bool to start the game
    gCurrentGame.m_gameStarted = true;
    gCurrentGame.m_gameRematch = false;
}

void Client_AcceptChallenge(string challenger_name)
{
    // -- create the player
    if (gCurrentGame.m_playerGroup.Used() < 2)
        gCurrentGame.CreatePlayer(challenger_name, false, "24 24 0");

    // -- set the connected bool
    gCurrentGame.m_isConnected = true;

    // -- set the bool to notify that the game has started
    gCurrentGame.m_gameStarted = true;
    gCurrentGame.m_gameRematch = false;
}

void Host_NotifyPlayerMove(int direction)
{
    object challenger = gCurrentGame.m_playerGroup.GetObjectByIndex(1);
    if (IsObject(challenger))
    {
        challenger.SetDirection(direction);
    }
}

void Client_NotifyPlayerUpdate(int packet_index, bool is_local_player, vector3f position, int length)
{
    // -- find the player
    int player_index = 0;
    if (!is_local_player)
        player_index = 1;
    object player = gCurrentGame.m_playerGroup.GetObjectByIndex(player_index);
    player.position = position;
    player.length = length;
    player.NotifyPosition();

    // -- let the host know we received the update
    SocketCommand("Host_NotifyPlayerUpdateResponse", packet_index);
}

void Host_NotifyPlayerUpdateResponse(int packet_index)
{
    if (packet_index >= gCurrentGame.m_requireResponseID)
    {
        gCurrentGame.m_requireResponseID = 0;
    }
}

void Client_NotifyClearApples()
{
    gCurrentGame.m_clientAppleCount = 0;
}

void Client_NotifyApple(vector3f apple_position)
{
    if (gCurrentGame.m_clientAppleCount < 16)
    {
        gCurrentGame.m_clientApples[gCurrentGame.m_clientAppleCount++] = apple_position;
    }
}

void NotifyGameOver(bool you_win, bool is_tie)
{
    // -- set the bools
    gCurrentGame.m_gameStarted = false;
    gCurrentGame.m_gameRematch = true;
    object challenger = gCurrentGame.m_playerGroup.GetObjectByIndex(1);

    if (is_tie)
    {
        ++gCurrentGame.m_localPlayer.m_score;
        if (IsObject(challenger))
            ++challenger.m_score;

        DrawText(9000, '320 240 0', "Tie Game!", gCOLOR_BLUE);
    }

    else if (!you_win)
    {
        if (IsObject(challenger))
            ++challenger.m_score;

        string lose_string;
        object challenger = gCurrentGame.m_playerGroup.GetObjectByIndex(1);
        if (IsObject(challenger))
            lose_string = StringCat(challenger.GetObjectName(), " Wins!");
        else
            lose_string = StringCat("You Lose!");
        DrawText(9000, '320 240 0', lose_string, gCOLOR_RED);
    }
    else
    {
        ++gCurrentGame.m_localPlayer.m_score;
        DrawText(9000, '320 240 0', "You Win!", gCOLOR_GREEN);
    }

    // -- draw the scores
    string my_score = StringCat(gCurrentGame.m_localPlayer.GetObjectName(), ": ", gCurrentGame.m_localPlayer.m_score);
    DrawText(9000, '200 10 0', my_score, gCurrentGame.m_isHost ? gCOLOR_BLUE : gCOLOR_GREEN);
    if (IsObject(challenger))
    {
        string challenger_score = StringCat(challenger.GetObjectName(), ": ", challenger.m_score);
        DrawText(9000, '200 30 0', challenger_score, gCurrentGame.m_isHost ? gCOLOR_GREEN : gCOLOR_BLUE);
    }

    if (gCurrentGame.m_isHost)
        gCurrentGame.DialogPlayAgain();
}

void Client_NotifyRematchRequest()
{
    gCurrentGame.DialogPlayAgain();
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
