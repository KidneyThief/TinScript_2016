// ====================================================================================================================
// asteroids.ts
// ====================================================================================================================

// -- this game is built upon the TinScriptDemo
Include("TinScriptDemo.ts");

// -- global tunables -------------------------------------------------------------------------------------------------
float gThrust = 5.0f;
float gAsteroidSpeed = 52.0f;
float gBulletSpeed = 150.0f;
float g_rotateSpeed = 2.0f;

int gMaxBullets = 4;
float gFireCDTime = 0.1f;

object gCurrentGame;

// -- FLTK Colors -----------------------------------------------------------------------------------------------------
int gCOLOR_BLACK = 0xff000000;
int gCOLOR_RED = 0xffff0000;
int gCOLOR_GREEN = 0xff00ff00;
int gCOLOR_YELLOW = 0xffff8f00;
int gCOLOR_BLUE = 0xff0000ff;
int gCOLOR_PURPLE = 0xffff00ff;
int gCOLOR_CYAN = 0xff00ffff;
int gCOLOR_BROWN = 0xff8fff00;

// ====================================================================================================================
// Asteroid : SceneObject implementation
// ====================================================================================================================
void Asteroid::OnCreate() : SceneObject
{
    // -- asteroids have movement
    vector3f self.velocity = '0 0 0';
    
    // -- add ourself to the game's asteroid set
    if (IsObject(gCurrentGame))
        gCurrentGame.asteroid_set.AddObject(self);
}

void Asteroid::OnUpdate(float deltaTime)
{
    // -- update the screen position - applies the velocity, and wraps
    UpdateScreenPosition(self, deltaTime);
        
    // -- draw the asteroid
    CancelDrawRequests(self);
    DrawCircle(self, self.position, self.radius, gCOLOR_BLACK);
}

void Asteroid::OnCollision()
{
    // -- get the asteroid heading
    vector3f velocity = self.velocity;
    float speed = V3fLength(velocity);
    float heading = Atan2(velocity:y, velocity:x);
    vector3f new_heading0 = '0 0 0';
    new_heading0:x = Cos(heading + 20.0f + (Random() * 40.0f));
    new_heading0:y = Sin(heading + 20.0f + (Random() * 40.0f));
    
    vector3f new_heading1 = '0 0 0';
    new_heading1:x = Cos(heading - 20.0f - (Random() * 40.0f));
    new_heading1:y = Sin(heading - 20.0f - (Random() * 40.0f));
    
    // -- if it's a large asteroid (radius == 50), split into two 30s
    bool should_split = (self.radius == 50 || self.radius == 30);
    float new_radius = 30;
    float new_speed_scale = 1.5f;
    if (self.radius == 30)
    {
        new_radius = 15;
        new_speed_scale = 2.5f;
    }
    
    if (should_split)
    {
        // -- two new asteroids, each spawned from the original position
        // -- each heading in a split direction, up to 90deg, but at twice the speed
        object asteroid0 = CreateSceneObject("Asteroid", self.position, new_radius);
        ApplyImpulse(asteroid0, new_heading0 * (speed * new_speed_scale));
        
        object asteroid1 = CreateSceneObject("Asteroid", self.position, new_radius);
        ApplyImpulse(asteroid1, new_heading1 * (speed * new_speed_scale));
        
        // -- and delete the original asteroid
        destroy self;
    }
    
    // -- otherwise, we simply destroy the object hit
    else
    {
        destroy self;

        // -- if this is the last asteroid, we win
        object asteroid_set = FindObject("AsteroidSet");
        if (IsObject(asteroid_set) && asteroid_set.Used() == 0)
        {
            DrawText(0, '320 240 0', "Y O U   W I N", gCOLOR_RED);
            
            SimPause();
            Print("Type StartAsteroids(); to continue...");
        }
    }
}

// ====================================================================================================================
// SpawnAsteroid():  Create an 'Asteroid' SceneObject at a random location, not in the center
// ====================================================================================================================
object SpawnAsteroid()
{
    // -- calculate a random spawn direction
    vector3f offset_dir = '0 0 0';
    offset_dir:x = -1.0f + 2.0f * Random();
    offset_dir:y = -1.0f + 2.0f * Random();
    offset_dir = V3fNormalized(offset_dir);
    
    // -- now we need a random distance, > 140.0f so it's not in the center of the arena
    float offset_dist = 140.0f + 100.0f * Random();
    
    // -- the actual spawn position is from the center, in the offset_dir, at the random length
    vector3f spawn_pos = '320 240 0' + (offset_dir * offset_dist);
    
    object new_asteroid = CreateSceneObject("Asteroid", spawn_pos, 50);
    
    // -- random impulse
    vector3f impulse = '0 0 0';
    impulse:x = -1.0f + 2.0f * Random();
    impulse:y = -1.0f + 2.0f * Random();
    impulse = V3fNormalized(impulse);
    
    // -- apply a random strength
    float strength = (gAsteroidSpeed * 0.2f) + ((gAsteroidSpeed * 0.8f) * Random());
    impulse *= strength;
    
    // -- random 
    ApplyImpulse(new_asteroid, impulse);

    return (new_asteroid);
}

void ApplyImpulse(object obj, vector3f impulse)
{
    if (IsObject(obj))
    {
        obj.velocity = obj.velocity + impulse;
    }
}

void UpdateScreenPosition(object obj, float deltaTime)
{
    if (IsObject(obj))
    {
        // -- apply the velocity
        obj.position = obj.position + (obj.velocity * deltaTime);

        // -- wrap the object around the screen edge
        if (obj.position:x < 0.0f)
            obj.position:x = 640.0f;
        else if (obj.position:x > 640.0f)
            obj.position:x = 0.0f;
            
        if (obj.position:y < 0.0f)
            obj.position:y = 480.0f;
        else if (obj.position:y > 480)
            obj.position:y = 0.0f;
    }
}

// ====================================================================================================================
// Ship implementation
// ====================================================================================================================
void Ship::OnCreate() : SceneObject
{
    // -- ships have velocity and rotation
    vector3f self.velocity = '0 0 0';
    float self.rotation = 270.0f;
    
    // -- life count
    int self.lives = 5;
    float self.show_hit_cd = 0;
    vector3f self.show_hit_position;
    
    // -- on spawn, we're invulnerable for a couple seconds
    float self.invulnerable_cd = 2.0f;
    
    // -- we can only fire so fast
    float self.fire_cd_time = 0.0f;
}

void Ship::OnUpdate(float deltaTime)
{
    // -- update the screen position - applies the velocity, and wraps
    UpdateScreenPosition(self, deltaTime);

    // -- update the CD times
    self.fire_cd_time -= deltaTime;
    self.invulnerable_cd -= deltaTime;
    
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
    
    // -- draw the 4 lines creating the "triangle-ish" ship
    DrawLine(self, head, tail0, gCOLOR_BLUE);
    DrawLine(self, head, tail1, gCOLOR_BLUE);
    DrawLine(self, tail0, self.position, gCOLOR_BLUE);
    DrawLine(self, tail1, self.position, gCOLOR_BLUE);
    
    // -- draw if we're hit
    if (self.show_hit_cd > 0.0f)
    {
        self.show_hit_cd -= deltaTime;
        DrawText(self, self.show_hit_position, "OUCH!", gCOLOR_RED);
    }
}

void Ship::ApplyThrust(float thrust)
{
    // -- calculate our heading
    vector3f heading = '0 0 0';
    heading:x = Cos(self.rotation);
    heading:y = Sin(self.rotation);
    
    // -- Apply the an impulse in the direction we're heading
    ApplyImpulse(self, heading * thrust);
}

void Ship::OnFire()
{
    // -- no shooting when we're dead
    if (self.lives <= 0)
        return;
        
    // -- only allow one bullet every X msec
    if (self.fire_cd_time > 0.0f)
        return;

    // -- only 4x bullets at a time
    if (gCurrentGame.bullet_set.Used() >= gMaxBullets)
        return;

    // -- calculate our heading
    vector3f heading = '0 0 0';
    heading:x = Cos(self.rotation);
    heading:y = Sin(self.rotation);
    
    // -- find the nose of the ship
    vector3f head = self.position + (heading * self.radius);
    
    // -- spawn a bullet
    SpawnBullet(head, heading);
    
    // -- start a cooldown
    self.fire_cd_time = gFireCDTime;
}

void Ship::OnCollision()
{
    // -- do nothing if we're invulnerable
    if (self.invulnerable_cd > 0.0f)
        return;
        
    // -- decrement the lives
    self.lives -= 1;
    
    // -- if we're dead, simply restart the entire game
    if (self.lives <= 0)
    {
        // -- game over, at the center of the screen
        DrawText(self, '320 240 0', "G A M E   O V E R", gCOLOR_RED);
        
        SimPause();
        Print("Type StartAsteroids(); to continue...");
        return;
    }
    
    // -- if we're dead, game over
    self.show_hit_cd = 1.5f;
    self.show_hit_position = self.position;
    
    // -- set ourselves invulnerable
    self.invulnerable_cd = 0.5f;
}

object SpawnShip()
{
    object new_ship = CreateSceneObject("Ship", '320 240 0', 20);
    return (new_ship);
}

// ====================================================================================================================
// Bullet implementation
// ====================================================================================================================
void Bullet::OnCreate() : SceneObject
{
    // -- Bullets have velocity
    vector3f self.velocity = '0 0 0';
    
    // -- self terminating
    float self.expireTime = 2.0f;
}

void Bullet::OnUpdate(float deltaTime)
{
    // -- update the screen position - applies the velocity, and wraps
    UpdateScreenPosition(self, deltaTime);
    
    // -- we're using our object ID also as a draw request ID
    CancelDrawRequests(self);
    DrawCircle(self, self.position, self.radius, gCOLOR_RED);
    
    // -- see if it's time to expire
    self.expireTime -= deltaTime;
    if (self.expireTime <= 0.0f)
        destroy self;
}

object SpawnBullet(vector3f position, vector3f direction)
{
    object bullet = CreateSceneObject("Bullet", position, 3);
    
    // -- apply the muzzle velocity
    ApplyImpulse(bullet, direction * gBulletSpeed);
    
    // -- add the bullet to the game's bullet set
    if (IsObject(gCurrentGame))
        gCurrentGame.bullet_set.AddObject(bullet);

    return (bullet);
}

// ====================================================================================================================
// Asteroids Game implementation
// ====================================================================================================================
void AsteroidsGame::OnCreate() : DefaultGame
{
    // -- create a set for the bullets (set == non-ownership)
    object self.asteroid_set = create CObjectSet("AsteroidSet");
    
    // -- create a set for the bullets (set == non-ownership)
    object self.bullet_set = create CObjectSet("BulletSet");
    
    // -- generic set, used for deleting bullets/asteroids/etc... things that have collided during OnUpdate
    object self.delete_set = create CObjectSet("DeleteSet");
    
    // -- cache the 'ship' object
    object self.ship;

    // -- set the update time
    int self.update_time = 0;
}

// -- the schedule to call this happens in DefaultGame::OnInit()
void AsteroidsGame::OnInit()
{
    DefaultGame::OnInit();
    
    self.asteroid_set = FindObject("AsteroidSet");
    if (!IsObject(self.asteroid_set))
        self.asteroid_set = create CObjectSet("AsteroidSet");
        
    self.bullet_set = FindObject("BulletSet");
    if (!self.bullet_set)
        self.bullet_set = create CObjectSet("BulletSet");
        
    self.delete_set = FindObject("DeleteSet");
    if (!IsObject(self.delete_set))
        self.delete_set = create CObjectSet("DeleteSet");
        
    // -- hook up the ship, however, this is only created from StartAsteroids(), not the OnCreate()
    self.ship = FindObject("Ship");
}

void AsteroidsGame::OnUpdate()
{
    // -- update all the scene objects
    DefaultGame::OnUpdate();

    // -- update the inputs
    self.UpdateKeys(self.update_time);
    self.update_time = GetSimTime();

    object bullet = self.bullet_set.First();
    while (IsObject(bullet))
    {
        // -- loop through all the asteroids
        object asteroid = self.asteroid_set.First();
        while (IsObject(asteroid))
        {
            // -- if the bullet is within radius of the asteroid, it's a collision
            float distance = V3fLength(bullet.position - asteroid.position);
            if (distance < asteroid.radius)
            {
                // -- post a notification
                if (IsFunction("PostCollision"))
                {
                    PostCollision(asteroid, bullet);
                }

                // -- set the flag, call the function
                asteroid.OnCollision();
                
                // -- add the bullet to the delete set and break
                self.delete_set.AddObject(bullet);
                
                // -- no need to keep colliding with this asteroid
                break;
            }
            
            // -- otherwise, get the next asteroid
            else
            {
                asteroid = self.asteroid_set.Next();
            }
        }
        
        // -- check the next bullet
        bullet = self.bullet_set.Next();
    }
    
    // -- look for asteroid collisions with the ship
    object asteroid = self.asteroid_set.First();
    while (IsObject(asteroid))
    {
        // -- if the distance to from the ship to the asteroid < sum of their radii
        float distance = V3fLength(asteroid.position - self.ship.position);
        float max_dist = (asteroid.radius + self.ship.radius) * 0.75f;
        if (distance < max_dist)
        {
            // -- post a notification
            if (IsFunction("PostCollision"))
            {
                PostCollision(asteroid, self.ship);
            }

            // -- notify the ship of the collision
            self.ship.OnCollision();
            
            // -- also split the asteroid
            asteroid.OnCollision();
            
            // -- note - the asteroid could have been deleted, so the loop should exit without
            // -- continuing to iterate
            break;
        }
        
        // -- else check the next asteroid
        else
        {
            asteroid = self.asteroid_set.Next();
        }
    }
    
    // -- now clean up the delete set
    while (self.delete_set.Used() > 0)
    {
        object delete_me = self.delete_set.GetObjectByIndex(0);
        destroy delete_me;

        // uncomment this to see attaching the IDE *after* an assert has triggered
        //Print(1 / 0);
        //ensure(false, "### DEBUG test Assert()");
    }
}

void AsteroidsGame::OnDestroy()
{
    // -- clean up the extra sets
    destroy self.asteroid_set;
    destroy self.bullet_set;
    destroy self.delete_set;
}

void AsteroidsGame::UpdateKeys(int update_time)
{
    if (!IsObject(self.ship))
        return;

    // -- normalize to 1/60th...
    float framerate_scale = self.DeltaTime / 0.016f;
    float scaled_rotate = g_rotateSpeed * framerate_scale;
    float scaled_thrust = gThrust * framerate_scale;

    // -- rotate left
    if (IsKeyPressed(KeyCode_J))
        self.ship.rotation -= scaled_rotate;
   
    // -- rotate right
    if (IsKeyPressed(KeyCode_L))
        self.ship.rotation += scaled_rotate;
   
    // -- thrust
    if (IsKeyPressed(KeyCode_I))
        self.ship.ApplyThrust(scaled_thrust);
   
    // -- fire
    if (KeyPressedSinceTime(KeyCode_space, update_time))
        self.ship.OnFire();
}

void StartAsteroids()
{
    ResetGame();
    gCurrentGame = create AsteroidsGame("CurrentGame");
    
    // -- spawn a ship
    gCurrentGame.ship = SpawnShip();
    
    // -- spawn, say, 8 asteroids
    schedule(0, 5000, hash("SpawnAsteroids"));
}

void SpawnAsteroids()
{
    int i;
    for (i = 0; i < 8; i += 1)
        SpawnAsteroid();
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
