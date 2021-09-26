// --------------------------------------------------------------------------------------------------------------------
// fltktools.ts
// Test script to demonstrate populating the debugger tool palette elements to support the FLTK demo.
// --------------------------------------------------------------------------------------------------------------------

SocketExec(hash("ToolPaletteClear"), "Asteroids");
SocketExec(hash("ToolPaletteAddMessage"), "Asteroids", "Demo Buttons");
SocketExec(hash("ToolPaletteAddButton"), "Asteroids", "Tools", "Execute demotools.ts", "Tools", "Exec('demotools.ts');");
SocketExec(hash("ToolPaletteAddButton"), "Asteroids", "Exec", "Execute asteroids.ts", "Exec Asteroids", "Exec('asteroids.ts');");
SocketExec(hash("ToolPaletteAddButton"), "Asteroids", "Start", "Start the Asteroids game", "Start", "StartAsteroids();");
SocketExec(hash("ToolPaletteAddButton"), "Asteroids", "Pause", "Pause the simulation", "Pause", "SimPause();");
SocketExec(hash("ToolPaletteAddButton"), "Asteroids", "Unpause", "Unpause the simulation", "Unpause", "SimUnpause();");

void TogglePaused()
{
    if (SimIsPaused())
    {
        SimUnpause();
        SocketExec(hash("ToolPaletteSetEntryValue"), "Asteroids::Toggle", "Pause");
        SocketExec(hash("ToolPaletteSetEntryDescription"), "Asteroids::Toggle", "The simulation is running.");
    }
    else
    {
        SimPause();
        SocketExec(hash("ToolPaletteSetEntryValue"), "Asteroids::Toggle", "Unpause");
        SocketExec(hash("ToolPaletteSetEntryDescription"), "Asteroids::Toggle", "The simulation is paused.");
    }
}
SocketExec(hash("ToolPaletteAddButton"), "Asteroids", "Toggle", "Toggle the paused state of the simulation.", "Pause", "TogglePaused();");

void SetTimeScale(int time_ticks)
{
    float new_time_scale = time_ticks;
    new_time_scale /= 100.0f;
    SimSetTimeScale(new_time_scale);
    string new_description = StringCat("[", time_ticks, "%]: Set the simulation time scale.");
    SocketExec(hash("ToolPaletteSetEntryDescription"), "Asteroids::TimeScale", new_description);
}
SocketExec(hash("ToolPaletteAddSlider"), "Asteroids", "TimeScale", "[100%]: Set the simulation time scale", 1, 400, 100, "SetTimeScale");
