// ====================================================================================================================
// MissionTree.ts
// ====================================================================================================================
Print("Executing MissionTree.ts");

// -- globals ---------------------------------------------------------------------------------------------------------

object gCurrentGame;

int STATUS_NONE = 0;
int STATUS_PENDING = 1;
int STATUS_SUCCESS = 2;
int STATUS_FAIL = 3;

// ====================================================================================================================
// MissionObject implementation
// ====================================================================================================================
LinkNamespaces("MissionGroup", "CObjectGroup");
void MissionAction::OnCreate()
{
    int self.status = STATUS_NONE;
    
    // -- mission actions can call Begin() on another mission action, on either success or fail
    object self.on_success_action;
    object self.on_fail_action;
    
    // -- these objects need to be updated, add them to the game
    if (IsObject(gCurrentGame))
    {
        gCurrentGame.game_objects.AddObject(self);
    }
}

void MissionAction::SetSuccessAction(object target_action)
{
    self.on_success_action = target_action;
}

void MissionAction::SetFailAction(object target_action)
{
    self.on_fail_action = target_action;
}

void MissionAction::Begin()
{
    // -- by default, the status is success, as there's nothing "pending" in the base class, and nothing to fail.
    self.status = STATUS_SUCCESS;
    
    // -- status update
    Print("MISSION [", self, "] ", self.GetObjectName(), " BEGIN");
}

void MissionAction::Reset()
{
    // -- perform whatever cleanup, and reset the status to NONE
    self.status = STATUS_NONE;
    CancelDrawRequests(self);
}

void MissionAction::Complete(bool success)
{
    // -- opportunity to do any cleanup as well
    if (success)
    {
        Print("MISSION [", self, "] ", self.GetObjectName(), " SUCCESS");
        self.status = STATUS_SUCCESS;
        if (IsObject(self.on_success_action))
            self.on_success_action.Begin();
    }
    else
    {
        Print("MISSION [", self, "] ", self.GetObjectName(), " FAIL");
        self.status = STATUS_FAIL;
        if (IsObject(self.on_fail_action))
            self.on_fail_action.Begin();
    }
}

void MissionAction::OnUpdate(float deltaTime)
{
}

void MissionAction::UpdateStatus()
{
}

int MissionAction::GetStatus()
{
    return (self.status);
}

// ====================================================================================================================
// MissionGroup implementation
// ====================================================================================================================
LinkNamespaces("MissionGroup", "MissionAction");
void MissionGroup::OnCreate()
{
    // -- a MissionGroup is how an action can be a group
    MissionAction::OnCreate();
}

void MissionGroup::UpdateStatus()
{
    // -- if our status is anything other than pending, return it
    if (self.status != STATUS_PENDING)
        return (self.status);
        
    // -- loop through the actions of this group, and "combine" their results
    // -- "none" is the same as pending - a child with no result simply hasn't been started yet
    // -- "fail" for one means fail for all
    // -- if no fails, then "pending" for one means pending for all
    // -- "success for *all* means group success
    int status = STATUS_NONE;
    bool found_success = false;
    bool found_pending = false;
    object mission_action = self.First();
    while (IsObject(mission_action))
    {
        int status = mission_action.GetStatus();
        if (status == STATUS_FAIL)
        {
            self.status = STATUS_FAIL;
            return;
        }
        else if (status == STATUS_PENDING || status == STATUS_NONE)
        {
            status = STATUS_PENDING;
            found_pending = true;
        }
        else if (status == STATUS_SUCCESS)
        {
            found_success = true;
        }
        
        // -- get the next action
        mission_action = self.Next();
    }
    
    // -- update the status:
    // -- if we'd have found a fail, we'd already have updated and returned a fail status
    // -- if we found a pending, then we're also still pending
    // -- if none of our children are pending, none are failed, and we found a success, the return success
    if ((!found_pending) && found_success)
        self.status = STATUS_SUCCESS;
 }
 
 int MissionGroup::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- call Begin() on all actions owned by this group
    // -- (note: an action can also be a group)
    object mission_action = self.First();
    while (IsObject(mission_action))
    {
        mission_action.Begin();
        mission_action = self.Next();
    }
    
    // -- set our status
    self.status = STATUS_PENDING;
}

int MissionGroup::Reset()
{
    // -- call the base class
    MissionAction::Reset();
    
    // -- call Reset() on all actions owned by this group
    object mission_action = self.First();
    while (IsObject(mission_action))
    {
        mission_action.Reset();
        mission_action = self.Next();
    }
}

void MissionGroup::OnUpdate(float delta_time)
{
    // -- no update if we're not pending
    if (self.status != STATUS_PENDING)
        return;
    
    // -- call OnUpdate() on all actions owned by this group
    object mission_action = self.First();
    while (IsObject(mission_action))
    {
        mission_action.OnUpdate(delta_time);
        mission_action = self.Next();
    }
}

// ====================================================================================================================
// ActionTriggerVolume implementation
// ====================================================================================================================
LinkNamespaces("ActionTriggerVolume", "MissionAction");
void ActionTriggerVolume::OnCreate()
{
    // -- this is a mission action
    MissionAction::OnCreate();
    
    // -- we need a reference to a trigger volume - note, we may have multiple actions sharing the trigger volume
    object self.trigger_volume;
}

void ActionTriggerVolume::OnDestroy()
{
    // -- remove our reference to the trigger volume
}

void ActionTriggerVolume::Initialize(object trigger_volume)
{
    if (IsObject(trigger_volume) == false)
        return;
    
    // -- set the trigger volume member
    self.trigger_volume = trigger_volume;
}

void ActionTriggerVolume::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- set our status
    self.status = STATUS_PENDING;
    
    // -- add ourselves to the notification set for the trigger volume
    if (IsObject(self.trigger_volume))
    {
        self.trigger_volume.AddNotify(self);
    }
}

void ActionTriggerVolume::Reset()
{
    // -- call the base class
    MissionAction::Reset();
    
    // -- remove ourself from notification by the trigger volume
    if (IsObject(self.trigger_volume))
    {
        self.trigger_volume.RemoveNotify(self);
    }
}

void ActionTriggerVolume::Complete(bool success)
{
    // -- call the base class
    MissionAction::Complete(success);
    
    // -- remove ourself from notification by the trigger volume upon any completion
    if (IsObject(self.trigger_volume))
    {
        self.trigger_volume.RemoveNotify(self);
    }
}

void ActionTriggerVolume::OnUpdate(float deltaTime)
{
    // -- no update if we're not pending
    if (self.status != STATUS_PENDING)
        return;
    
    // -- update our trigger volume
    object trigger_volume = self.First();
    if (IsObject(trigger_volume))
    {
        trigger_volume.OnUpdate(deltaTime);
    }
}

void ActionTriggerVolume::NotifyOnEnter(object character)
{
}

void ActionTriggerVolume::NotifyOnExit(object character)
{
}

// ====================================================================================================================
// MissionTreeLinear implementation - children are run linearly, one after the other
// ====================================================================================================================
// -- a MissionTree is simply a mission action (group) that executes the next child who is pending
LinkNamespaces("MissionTreeLinear", "MissionGroup");
void MissionTreeLinear::OnCreate()
{
    // -- construct the base class
    MissionGroup::OnCreate();
}

// -- when the tree begins, all children are initialized to STATUS_NONE
// -- the first child is started
void MissionTreeLinear::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- set my status to pending
    self.status = STATUS_PENDING;
    
    // -- initialize all children to reset their status to NONE
    object child = self.First();
    while (IsObject(child))
    {
        child.Reset();
        child = self.Next();
    }
    
    // -- and begin the first child
    if (IsObject(self.First()))
        self.First().Begin();
}

// -- the update will find the next child who is pending, skipping all other children (including those who have failed)
// -- it's up to the owner of the mission tree to call Reset() if the tree succeeds or fails
void MissionTreeLinear::OnUpdate(float delta_time)
{
    // -- first update our status
    self.UpdateStatus();
    
    // -- nothing to update, if we're not pending
    if (self.status != STATUS_PENDING)
        return;
        
    // -- find the next child who is pending, and update them
    object child_to_update = self.First();
    while (IsObject(child_to_update))
    {
        // -- if we found our pending child, update
        if (child_to_update.status == STATUS_PENDING)
        {
            child_to_update.OnUpdate(delta_time);
            return;
        }
        
        // -- else if we found a child who hasn't yet been started, start
        else if (child_to_update.status == STATUS_NONE)
        {
            child_to_update.Begin();
            child_to_update.OnUpdate(delta_time);
            return;
        }
        
        // -- if neither is true, get the next child and keep looking
        child_to_update = self.Next();
    }
}

// ====================================================================================================================
// MissionTreeParallel implementation - children are run in parallel
// ====================================================================================================================
// -- a MissionTree is simply a mission action (group) that executes the next child who is pending
LinkNamespaces("MissionTreeParallel", "MissionGroup");
void MissionTreeParallel::OnCreate()
{
    // -- construct the base class
    MissionGroup::OnCreate();
}

// -- when the tree begins, all children are initialized to STATUS_NONE
// -- the first child is started
void MissionTreeParallel::Begin()
{
    // -- call the root class
    MissionAction::Begin();
    
    // -- now set my status to pending
    self.status = STATUS_PENDING;
    
    // -- call Begin() on all children
    object child = self.First();
    while (IsObject(child))
    {
        child.Begin();
        child = self.Next();
    }
}

// -- the update will find the next child who is pending, skipping all other children (including those who have failed)
// -- it's up to the owner of the mission tree to call Reset() if the tree succeeds or fails
void MissionTreeParallel::OnUpdate(float delta_time)
{
    // -- first update our status
    self.UpdateStatus();
    
    // -- nothing to update, if we're not pending
    if (self.status != STATUS_PENDING)
        return;
        
    // -- find the next child who is pending, and update them
    object child_to_update = self.First();
    while (IsObject(child_to_update))
    {
        // -- if we found our pending child, update
        if (child_to_update.status == STATUS_PENDING)
        {
            child_to_update.OnUpdate(delta_time);
        }
        
        // -- next child
        child_to_update = self.Next();
    }
}

// ====================================================================================================================
// EOF
// ====================================================================================================================
