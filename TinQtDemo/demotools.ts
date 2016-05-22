// --------------------------------------------------------------------------------------------------------------------
// fltktools.ts
// Test script to demonstrate populating the debugger tool palette elements to support the FLTK demo.
// --------------------------------------------------------------------------------------------------------------------

SocketCommand("ToolPaletteClear", "Asteroids");
SocketCommand("ToolPaletteAddMessage", "Asteroids", "Demo Buttons");
SocketCommand("ToolPaletteAddButton", "Asteroids", "Tools", "Execute demotools.ts", "Tools", "Exec('demotools.ts');");
SocketCommand("ToolPaletteAddButton", "Asteroids", "Exec", "Execute asteroids.ts", "Exec Asteroids", "Exec('asteroids.ts');");
SocketCommand("ToolPaletteAddButton", "Asteroids", "Start", "Start the Asteroids game", "Start", "StartAsteroids();");
SocketCommand("ToolPaletteAddButton", "Asteroids", "Pause", "Pause the simulation", "Pause", "SimPause();");
SocketCommand("ToolPaletteAddButton", "Asteroids", "Unpause", "Unpause the simulation", "Unpause", "SimUnpause();");

void TogglePaused()
{
    if (SimIsPaused())
    {
        SimUnpause();
        SocketCommand("ToolPaletteSetEntryValue", "Asteroids::Toggle", "Pause");
        SocketCommand("ToolPaletteSetEntryDescription", "Asteroids::Toggle", "The simulation is running.");
    }
    else
    {
        SimPause();
        SocketCommand("ToolPaletteSetEntryValue", "Asteroids::Toggle", "Unpause");
        SocketCommand("ToolPaletteSetEntryDescription", "Asteroids::Toggle", "The simulation is paused.");
    }
}
SocketCommand("ToolPaletteAddButton", "Asteroids", "Toggle", "Toggle the paused state of the simulation.", "Pause", "TogglePaused();");

void SetTimeScale(int time_ticks)
{
    float new_time_scale = time_ticks;
    new_time_scale /= 100.0f;
    SimSetTimeScale(new_time_scale);
    string new_description = StringCat("[", time_ticks, "%%]: Set the simulation time scale.");
    SocketCommand("ToolPaletteSetEntryDescription", "Asteroids::TimeScale", new_description);
}
SocketCommand("ToolPaletteAddSlider", "Asteroids", "TimeScale", "[100%%]: Set the simulation time scale", 1, 400, 100, "SetTimeScale");
