struct Foc
{
    FocState state;

} foc;

//======================================================
// CORE MODE API
//======================================================
void Foc_init();
void Foc_onEnter(MCMode prev_mct);
void Foc_onExit();
void Foc_pwmLoop();
void Foc_xmcLoop();
