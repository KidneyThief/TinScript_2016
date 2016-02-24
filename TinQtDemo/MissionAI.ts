// ====================================================================================================================
// MissionAI.ts
// ====================================================================================================================
Print("Executing MissionAI.ts");

object gCurrentGame;

float Character::GetRotationToTarget()
{
    if (!IsObject(self.target))
        return (0.0f);
        
    // -- always aim, say, 2.0f seconds into the future
    vector3f target_pos = self.target.position + self.target.velocity * self.prediction_time;
        
    vector3f v_to_target = target_pos - self.position;
    float heading_to_target = Atan2(v_to_target:y, v_to_target:x);
    float heading_diff = heading_to_target - self.rotation;
    
    // -- ensure a value between -180.0f and 180.0f
    while (heading_diff <= -180.0f)
        heading_diff += 360.0f;
        
    while (heading_diff > 180.0f)
        heading_diff -= 360.0f;
        
    return (heading_diff);
}

void Enemy::UpdateAI()
{
    // -- get my position
    vector3f my_pos = self.position;

    // -- find a target
    if (!IsObject(self.target) && IsObject(gCurrentGame.character_set))
    {
        // -- find the closest enemy
        object character_set = gCurrentGame.character_set;
        object test_character = character_set.First();
        object closest_target;
        float closest_dist = 10000.0f;
        while (IsObject(test_character))
        {
            if (test_character.team != TEAM_NONE && test_character.team != self.team && test_character.health > 0)
            {
                vector3f v_to_character = test_character.position - my_pos;
                float test_dist = V3fLength(v_to_character);
                if (test_dist < closest_dist)
                {
                    closest_target = test_character;
                    closest_dist = test_dist;
                }
            }
            
            // -- get the next
            test_character = character_set.Next();
        }
        
        // -- set the new target
        if (IsObject(closest_target))
            self.target = closest_target;
    }
    
    // -- aim at the target
    if (IsObject(self.target))
    {
        vector3f v_to_target = self.target.position - my_pos;
        float dist_to_target = V3fLength(v_to_target);
        vector3f v_to_target_norm = v_to_target;
        if (dist_to_target > 0.0f)
            v_to_target_norm /= dist_to_target;
        
        float heading_to_target = Atan2(v_to_target:y, v_to_target:x);
        
        float heading_diff = self.GetRotationToTarget();
        if (heading_diff >= self.rotation_speed)
            self.rotation += self.rotation_speed;
        else if (heading_diff > 0.0f)
            self.rotation += heading_diff;
        else if (heading_diff <= -(self.rotation_speed))
            self.rotation -= self.rotation_speed;
        else if (heading_diff < 0.0f)
            self.rotation += heading_diff;
            
        if (heading_diff < 10.0f)
        {
            float cur_speed = V3fLength(self.velocity);
            vector3f velocity_norm = self.velocity;
            if (cur_speed > 0.0f)
                velocity_norm /= cur_speed;
            float velocity_dot = V3fDot(velocity_norm, v_to_target_norm);
            float speed_percent = cur_speed / gEnemy_MaxSpeed;
            if (speed_percent < 0.25f)
                speed_percent = 0.25f;
                
            // -- fire!
            if (dist_to_target < 300.0f)
                self.OnFire();
            
            // -- thrust
            if (dist_to_target > gEnemy_MaxDist && (cur_speed < gEnemy_MaxSpeed || velocity_dot < 0.0f))
            {
                if (velocity_dot < 0.0f || cur_speed < gEnemy_MinSpeed)
                    speed_percent = 1.0f;
                self.ApplyThrust(gEnemy_Thrust * speed_percent * speed_percent);
            }
            else if (dist_to_target < gEnemy_MinDist && (cur_speed < gEnemy_MaxSpeed || velocity_dot > 0.0f))
            {
                if (velocity_dot > 0.0f || cur_speed < gEnemy_MinSpeed)
                    speed_percent = 1.0f;
                self.ApplyThrust(-gEnemy_Thrust * speed_percent * speed_percent);
            }
        }
    }
}
