// ====================================================================================================================
// MissionObjects.ts
// ====================================================================================================================

// -- this game is built upon the TinScriptDemo
Include("TinScriptDemo.ts");

// -- aux file just for the AI
Include("MissionAI.ts");
Include("MissionSpawns.ts");
Include("MissionTree.ts");
Include("MissionAction.ts");

// -- globals ---------------------------------------------------------------------------------------------------------

object gCurrentGame;

float gPlayer_Thrust = 3.0f;
float gAsteroidSpeed = 52.0f;
float gBulletSpeed = 150.0f;

int gMaxBullets = 4;

int gPlayer_FireCDTime = 100;
int gEnemy_FireCDTime = 2000;

int TEAM_NONE = 0;
int TEAM_ALLY = 1;
int TEAM_ENEMY = 2;
float gPlayer_Radius = 20.0f;
float gEnemy_Radius = 15.0f;

float gDefaultRotationSpeed = 2.0f;
float gEnemy_RotationSpeed = 1.0f;
float gPlayer_RotationSpeed = 1.5f;

float gEnemy_Thrust = 3.0f;
float gEnemy_MinSpeed = 10.0f;
float gEnemy_MaxSpeed = 60.0f;
float gEnemy_PredictionTime = 1.2f;
float gEnemy_MinDist = 120.0f;
float gEnemy_MaxDist = 150.0f;

// -- Colors ----------------------------------------------------------------------------------------------------------
int gCOLOR_BLACK = 0xff00ff00;
int gCOLOR_RED = 0xffff0000;
int gCOLOR_GREEN = 0xff00ff00;
int gCOLOR_YELLOW = 0xffff8f00;
int gCOLOR_BLUE = 0xff0000ff;
int gCOLOR_PURPLE = 0xffff00ff;
int gCOLOR_CYAN = 0xff00ffff;

// ====================================================================================================================
// Helper functions
// ====================================================================================================================
void UpdateScreenPosition(object obj, float deltaTime)
{
    if (IsObject(obj))
    {
        // -- apply the velocity
        obj.position = obj.position + (obj.velocity * deltaTime);

        // -- wrap the object around the screen edge
        if (obj.position:x < 0.0f)
        {
            obj.position:x = 0.0f;
            obj.velocity:x = 0.0f;
        }
        else if (obj.position:x > 640.0f)
        {
            obj.position:x = 640.0f;
            obj.velocity:x = 0.0f;
        }
            
        if (obj.position:y < 0.0f)
        {
            obj.position:y = 0.0f;
            obj.velocity:y = 0.0f;
        }
        else if (obj.position:y > 480)
        {
            obj.position:y = 480.0f;
            obj.velocity:y = 0.0f;
        }
    }
}

// ====================================================================================================================
// TriggerVolume implementation
// ====================================================================================================================
void TriggerVolume::OnCreate() : SceneObject
{
    // -- construct the base class
    SceneObject::OnCreate();
    
    vector3f self.top_left;
    vector3f self.bottom_right;
    
    // -- create a set of objects to notify, when an OnEnter / OnExit happens
    object self.notify_set = create CObjectSet("NotifySet");
    
    // -- create a set of characters currently within the volume
    object self.character_set = create CObjectSet("ContainsCharSet");
    
    // -- if we have an active game, add ourself to the trigger volume set
    if (IsObject(gCurrentGame))
    {
        gCurrentGame.triggervolume_set.AddObject(self);
    }
}

void TriggerVolume::OnDestroy()
{
    destroy self.notify_set;
    destroy self.character_set;
}

void TriggerVolume::Initialize(vector3f top_left, vector3f bottom_right)
{
    self.top_left = top_left;
    self.bottom_right = bottom_right;
}

bool TriggerVolume::ContainsPosition(vector3f position)
{
    if (position:x < self.top_left:x)
        return (false);
    if (position:x > self.bottom_right:x)
        return (false);
    if (position:y < self.top_left:y)
        return (false);
    if (position:y > self.bottom_right:y)
        return (false);
        
    // -- success
    return (true);
}

void TriggerVolume::OnUpdate(float deltaTime)
{
    // -- draw the trigger volume
    CancelDrawRequests(self);
    
    // -- if there's no one to receive the notifications, there's no point in updating
    if (self.notify_set.Used() == 0)
        return;

    int color = gCOLOR_BLUE;
    if (self.character_set.Used() > 0)
        color = gCOLOR_PURPLE;
        
    vector3f top_right = self.top_left;
    top_right:x = self.bottom_right:x;
    vector3f bottom_left = self.top_left;
    bottom_left:y = self.bottom_right:y;
        
    DrawLine(self, self.top_left, top_right, color);
    DrawLine(self, top_right, self.bottom_right, color);
    DrawLine(self, self.bottom_right, bottom_left, color);
    DrawLine(self, bottom_left, self.top_left, color);
    
    // -- loop through the TV's character set, see if a character who is inside the trigger volume has left
    object character = self.character_set.First();
    while (IsObject(character))
    {
        if (self.ContainsPosition(character.position) == false)
        {
            self.character_set.RemoveObject(character);
            self.OnExit(character);
        }
        
        // -- get the next character (and yes, the iterator is updated correctly even if we remove the last)
        character = self.character_set.Next();
    }
    
    // -- now loop through the current game's character set, see if there's a character who has entered
    if (IsObject(gCurrentGame.character_set))
    {
        object character = gCurrentGame.character_set.First();
        while (IsObject(character))
        {
            if (self.character_set.Contains(character) == false && self.ContainsPosition(character.position))
            {
                self.character_set.AddObject(character);
                self.OnEnter(character);
            }
            character = gCurrentGame.character_set.Next();
        }
    }
}

// -- adding to the notify set implies the requesting object has implemented NotifyOnEnter()/NotifyOnExit() methods
void TriggerVolume::AddNotify(object notify_me)
{
    if (IsObject(notify_me))
        self.notify_set.AddObject(notify_me);
}

// -- once the notify set is empty, it's essentially no longer active
void TriggerVolume::RemoveNotify(object notify_me)
{
    self.notify_set.RemoveObject(notify_me);
    if (self.notify_set.Used() == 0)
    {
        CancelDrawRequests(self);
        
        // -- we also want to completely clear the character_set, so any new events start from scratch
        self.character_set.RemoveAll();
    }
}

void TriggerVolume::OnEnter(object character)
{
    // -- notify the requesting objects
    object notify_me = self.notify_set.First();
    while (IsObject(notify_me))
    {
        if (ObjectHasMethod(notify_me, "NotifyOnEnter"))
        {
            notify_me.NotifyOnEnter(character);
        }
        notify_me = self.notify_set.Next();
    }
}

void TriggerVolume::OnExit(object character)
{
    // -- notify the requesting objects
    object notify_me = self.notify_set.First();
    while (IsObject(notify_me))
    {
        if (ObjectHasMethod(notify_me, "NotifyOnExit"))
        {
            notify_me.NotifyOnExit(character);
        }
        notify_me = self.notify_set.Next();
    }
}

// ====================================================================================================================
// Character implementation
// ====================================================================================================================
void Character::OnCreate() : SceneObject
{
    // -- construct the base class
    SceneObject::OnCreate();
    
    // -- characters have velocity and rotation
    vector3f self.velocity = '0 0 0';
    float self.rotation = 270.0f;
    
    // -- life count
    int self.health = 20;
    int self.show_hit_time = 0;
    
    // -- team and target
    int self.team = TEAM_NONE;
    object self.target;
    float self.rotation_speed = gDefaultRotationSpeed;
    object self.notify_death_set = create CObjectSet("NotifyDeath");
    
    // -- we can only fire so fast
    int self.fire_cd_time = 0;
    int self.fire_cd = gPlayer_FireCDTime;
    object self.bullet_set = create CObjectSet("BulletSet");
    
    // -- add ourself to the game's character set
    if (IsObject(gCurrentGame))
    {
        gCurrentGame.character_set.AddObject(self);
    }
}

void Character::OnDestroy()
{
    SceneObject::OnDestroy();
    destroy self.bullet_set;
    destroy self.notify_death_set;
}

void Character::AddNotifyOnKilled(object listener)
{
    if (IsObject(listener))
        self.notify_death_set.AddObject(listener);
}

void Character::RemoveNotifyOnKilled(object listener)
{
    if (IsObject(listener))
        self.notify_death_set.RemoveObject(listener);
}

void Character::OnUpdate(float deltaTime)
{
    // -- update the screen position - applies the velocity, and wraps
    UpdateScreenPosition(self, deltaTime);
    
    // -- we look like a triangle
    float head_offset_x = self.radius * Cos(self.rotation);
    float head_offset_y = self.radius * Sin(self.rotation);
    vector3f head = self.position;
    head:x = head:x + head_offset_x;
    head:y = head:y + head_offset_y;
    
    float tail0_offset_x = self.radius * Cos(self.rotation + 120.0f);
    float tail0_offset_y = self.radius * Sin(self.rotation + 120.0f);
    vector3f tail0 = self.position;
    tail0:x += tail0_offset_x;
    tail0:y += tail0_offset_y;

    float tail1_offset_x = self.radius * Cos(self.rotation + 240.0f);
    float tail1_offset_y = self.radius * Sin(self.rotation + 240.0f);
    vector3f tail1 = self.position;
    tail1:x += tail1_offset_x;
    tail1:y += tail1_offset_y;
    
    // -- cancel all draws for this
    CancelDrawRequests(self);
    
    // -- determine our color
    int color = gCOLOR_BLACK;
    if (self.team == TEAM_ALLY)
        color = gCOLOR_BLUE;
    else if (self.team == TEAM_ENEMY)
        color = gCOLOR_PURPLE;
    int cur_time = GetSimTime();
    if (cur_time < self.show_hit_time || self.health <= 0)
        color = gCOLOR_RED;
    
    // -- draw the 4 lines creating the "triangle-ish" ship
    DrawLine(self, head, tail0, color);
    DrawLine(self, head, tail1, color);
    DrawLine(self, tail0, self.position, color);
    DrawLine(self, tail1, self.position, color);
    
    // -- if we're dead, draw the text
    if (self.health <= 0)
        DrawText(self, self.position - '15 0 0', "DEAD", gCOLOR_RED);
}

void Character::ApplyThrust(float thrust)
{
    // -- calculate our heading
    vector3f heading = '0 0 0';
    heading:x = Cos(self.rotation);
    heading:y = Sin(self.rotation);
    
    // -- if we're thrusting in the opposite direction, apply the break once
    float cur_speed = V3fLength(self.velocity);
    vector3f velocity_norm = self.velocity;
    if (cur_speed > 0.0f)
        velocity_norm /= cur_speed;
    float velocity_dot = V3fDot(velocity_norm, heading);
    if (velocity_dot < -0.70f)
    {
        self.ApplyBreak();
    }
    
    // -- Apply the an impulse in the direction we're heading
    ApplyImpulse(self, heading * thrust);
}

void Character::ApplyBreak()
{
    // -- dampen the velocity
    vector3f cur_velocity = self.velocity;
    float cur_speed = V3fLength(cur_velocity);
    if (cur_speed > 0.0f)
    {
        cur_velocity /= cur_speed;
        cur_velocity = cur_velocity * cur_speed * 0.5f;
        self.velocity = cur_velocity;
    }
}

void Character::OnFire()
{
    // -- no shooting when we're dead
    if (self.health <= 0)
        return;
        
    // -- only 4x bullets at a time
    if (self.bullet_set.Used() >= gMaxBullets)
        return;
    
    // -- only allow one bullet every X msec
    int cur_time = GetSimTime();
    if (cur_time < self.fire_cd_time)
        return;
        
    // -- calculate our heading
    vector3f heading = '0 0 0';
    heading:x = Cos(self.rotation);
    heading:y = Sin(self.rotation);
    
    // -- find the nose of the ship
    vector3f head = self.position + (heading * self.radius);
    
    // -- spawn a bullet
    object bullet = SpawnBullet(self, head, heading);
    self.bullet_set.AddObject(bullet);
    
    // -- start a cooldown
    self.fire_cd_time = cur_time + self.fire_cd;
}

void Character::OnDeath()
{
    destroy self;
}

void Character::OnCollision(object bullet)
{
    if (self.health <= 0)
        return;
    
    // -- decrement the health
    self.health -= 1;
    
    // -- if we're dead, schedule the object to be deleted
    if (self.health <= 0)
    {
        // -- draw the "DEAD" immediately
        DrawText(self, self.position - '15 0 0', "DEAD", gCOLOR_RED);
        
        // -- notify whoever wants to know about our death
        object listener = self.notify_death_set.First();
        while (IsObject(listener))
        {
            if (ObjectHasMethod(listener, "NotifyOnKilled"))
                listener.NotifyOnKilled(self);
            listener = self.notify_death_set.Next();
        }
        
        // -- schedule the destruction
        schedule(self, 2000, Hash("OnDeath"));
        return;
    }
    
    ApplyImpulse(self, bullet.velocity * 0.5f);
    
    self.show_hit_time = GetSimTime() + 500;
}

// ====================================================================================================================
// Bullet implementation
// ====================================================================================================================
void Bullet::OnCreate() : SceneObject
{
    // -- construct the base class
    SceneObject::OnCreate();
    
    // -- Bullets are shot by characters
    object self.source;
    
    // -- Bullets have velocity
    vector3f self.velocity = '0 0 0';
    
    // -- self terminating
    int cur_time = GetSimTime();
    int self.expireTime = cur_time + 2000;
}

void Bullet::OnUpdate(float deltaTime)
{
    // -- update the screen position - applies the velocity, and wraps
    UpdateScreenPosition(self, deltaTime);
    
    // -- we're using our object ID also as a draw request ID
    CancelDrawRequests(self);
    DrawCircle(self, self.position, self.radius, gCOLOR_RED);
    
    // -- see if it's time to expire
    int cur_time = GetSimTime();
    if (cur_time > self.expireTime)
        destroy self;
}

object SpawnBullet(object source, vector3f position, vector3f direction)
{
    object bullet = CreateSceneObject("Bullet", position, 3);
    bullet.source = source;
    
    // -- apply the muzzle velocity
    ApplyImpulse(bullet, direction * gBulletSpeed);
    
    // -- add the bullet to the game's bullet set
    if (IsObject(gCurrentGame))
        gCurrentGame.bullet_set.AddObject(bullet);
        
    return (bullet);
}

// ====================================================================================================================
// MissionSim implementation
// ====================================================================================================================
void MissionSim::OnCreate() : DefaultGame
{
    // -- construct the base class
    DefaultGame::OnCreate();

    // -- update time
    int self.update_time = 0;
    
    // -- cache the 'player' object
    object self.player;
    
    // -- if a "dialog" is displayed, set an object to receive the notification that "OK" was pressed
    object self.notify_dialog_ok;
    
    // -- create sets for iterating/finding objects
    object self.character_set = create CObjectSet("CharacterSet");
    object self.bullet_set = create CObjectSet("BulletSet");
    object self.delete_set = create CObjectSet("DeleteSet");
    object self.spawnpoint_set = create CObjectSet("SpawnpointSet");
    object self.triggervolume_set = create CObjectSet("TriggerVolumeSet");
}

void MissionSim::OnDestroy()
{
    // -- call the default
    DefaultGame::OnDestroy();
    
    // -- clean up the extra sets
    destroy self.character_set;
    destroy self.bullet_set;
    destroy self.delete_set;
    destroy self.spawnpoint_set;
    destroy self.triggervolume_set;
}

void MissionSim::OnUpdate()
{
    // -- update all the scene objects
    DefaultGame::OnUpdate();

    MissionSim::UpdateKeys();
    
    object bullet = self.bullet_set.First();
    while (IsObject(bullet))
    {
        // -- loop through all the asteroids
        bool finished = false;
        object character = self.character_set.First();
        while (!finished && IsObject(character))
        {
            if (character.team != TEAM_NONE && bullet.source.team != character.team)
            {
                // -- if the bullet is within radius of the character, it's a collision
                float distance = V3fLength(bullet.position - character.position);
                if (distance < character.radius)
                {
                    // -- set the flag, call the function
                    character.OnCollision(bullet);
                    
                    // -- add the bullet to the delete set and break
                    self.delete_set.AddObject(bullet);
                    
                    // $$$TZA break doesn't compile??
                    finished = true;
                }
            }
            
            // -- get the next character
            character = self.character_set.Next();
        }
        
        // -- check the next bullet
        bullet = self.bullet_set.Next();
    }
    
    // -- now clean up the delete set
    while (self.delete_set.Used() > 0)
    {
        object delete_me = self.delete_set.GetObjectByIndex(0);
        destroy delete_me;
    }
}

void MissionSim::OnKeyEvent(int key_code, bool pressed)
{
    // -- only handle presses
    if (!pressed || !IsObject(gCurrentGame.player))
        return;

    // -- fire
    if (key_code == KeyCode_space)
    {
        gCurrentGame.player.OnFire();
    }
    
    // -- pause / unpause
    if (key_code == KeyCode_esc)
    {
        bool is_paused = SimIsPaused();
        if (is_paused)
            SimUnpause();
        else
            SimPause();
    }
    
    // -- "dialog OK" button
    if (key_code == KeyCode_enter)
    {
        if (ObjectHasMethod(gCurrentGame.notify_dialog_ok, "OnDialogOK"))
            gCurrentGame.notify_dialog_ok.OnDialogOK();
    }
}

void MissionSim::UpdateKeys()
{
    // -- ensure we have a valid player
    if (!IsObject(self.player))
        return;

    // -- rotate left
    if (IsKeyPressed(KeyCode_J))
    {
        gCurrentGame.player.rotation -= gPlayer_RotationSpeed;
    }
    
    // -- rotate right
    if (IsKeyPressed(KeyCode_L))
    {
        gCurrentGame.player.rotation += gPlayer_RotationSpeed;
    }
    
    // -- thrust
    if (IsKeyPressed(KeyCode_I))
    {
        gCurrentGame.player.ApplyThrust(gPlayer_Thrust);
    }
    
    // -- break
    if (IsKeyPressed(KeyCode_K))
    {
        gCurrentGame.player.ApplyBreak();
    }
    
    // -- update the time
    self.update_time = GetSimTime();
}

void MissionSim::SetNotifyDialogOK(object notify_me)
{
    gCurrentGame.notify_dialog_ok = notify_me;
}

void ApplyImpulse(object obj, vector3f impulse)
{
    if (IsObject(obj))
    {
        obj.velocity = obj.velocity + impulse;
    }
}

// ====================================================================================================================
// Player
// ====================================================================================================================
void Player::OnCreate() : Character
{
    // -- construct the base class
    Character::OnCreate();
    
    // -- set the team
    self.team = TEAM_ALLY;
    
    // -- let the game know who to send key events to
    gCurrentGame.player = self;
    
    // -- override the radius
    self.radius = gPlayer_Radius;
}

void Player::OnUpdate(float deltaTime)
{
    Character::OnUpdate(deltaTime);
}

// ====================================================================================================================
// Enemy
// ====================================================================================================================
void Enemy::OnCreate() : Character
{
    // -- construct the base class
    Character::OnCreate();
    
    // -- set the team and health
    self.team = TEAM_ENEMY;
    self.health = 5;
    float self.prediction_time = gEnemy_PredictionTime * (0.5f + Random());
    
    // -- override the radius, fire CD time, and turning speed
    // -- apply a bit of randomness
    self.radius = gEnemy_Radius;
    self.fire_cd = gEnemy_FireCDTime * (0.5f + Random());
    self.rotation_speed = gEnemy_RotationSpeed * (0.5f + Random());
}

void Enemy::OnUpdate(float deltaTime)
{
    // -- update the base class
    Character::OnUpdate(deltaTime);
    
    // -- dampen the velocity
    vector3f cur_velocity = self.velocity;
    float cur_speed = V3fLength(cur_velocity);
    if (cur_speed > 0.0f)
    {
        cur_velocity /= cur_speed;
        cur_velocity = cur_velocity * cur_speed * 0.995f;
        self.velocity = cur_velocity;
    }
    
    // -- update the AI
    if (self.health > 0)
        self.UpdateAI();
}

// ====================================================================================================================
// Mission functions
// ====================================================================================================================
object SpawnCharacter(string char_name, vector3f position)
{
    if (!StringCmp(char_name, ""))
        char_name = "Character";
    object new_char = CreateSceneObject(char_name, position, 20);
    return (new_char);
}

// ====================================================================================================================
// EOF
// ====================================================================================================================


