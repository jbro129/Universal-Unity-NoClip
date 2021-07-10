# Universal-Unity-NoClip
This projects aim to show how a NoClip mod can be created in any unity game, regardless if its using an il2cpp or mono back-end.

## Requirements
 - The game uses [CharacterController](https://docs.unity3d.com/ScriptReference/CharacterController.html) to control player movement and state.
 - You have to be able to use the [set_radius](https://docs.unity3d.com/ScriptReference/CharacterController-radius.html) function within the CharacterController class. If that method is stripped from your dump then you are out of luck.
 
 ## Il2CPP
 
For this example I am going to be using PG3D v16.6.1. PG3D uses CharacterController to handle everything player related. **This example applies only to PG3D, other games may implement CharacterController differently.**

For my initial C++ hook of pg3d, I am going to be hooking the UpdateEffects method of the class Player_move_c.

It is bad practice to make extra hooks, if you can do everything inside of one hook then you should. I hooked into Player_move_c because I can access a lot from there.

My goal is to be able to access the games CharacterController from my hook. 

On line 197626 of [dump.cs](https://github.com/jbro129/Universal-Unity-NoClip/blob/main/il2cpp/Dump/dump.cs) you will see that in class FirstPersonControllerCSharp has a field `internal CharacterController character; // 0x74`. This is where PG3D handles CharacterController.

If I want to be able to use FirstPersonControllerCSharp and its character field then I will have to find what other classes use the FirstPersonControllerCSharp class.

One of the classes that use FirstPersonControllerCSharp is the SkinName class. On line 189324 of [dump.cs](https://github.com/jbro129/Universal-Unity-NoClip/blob/main/il2cpp/Dump/dump.cs) you can see `public FirstPersonControlSharp firstPersonControl; // 0xD8`.

Now that we have found a class that is using FirstPersonControllerCSharp, we have to find a class, field, or method that is using that class. We have to repeat this until we find a way to access said function from our hook. It could be via a static class pointer or through a method or field.

For PG3D's case,  I can access SkinName from my initial hook. Player_move_c has a field that uses SkinName, on line 145297 of [dump.cs](https://github.com/jbro129/Universal-Unity-NoClip/blob/main/il2cpp/Dump/dump.cs) `internal SkinName mySkinName; // 0x320`.

Now that we have a way to access CharacterController from our hook, we need to implement a way to access it.

We need to write the C# code `this.mySkinName.firstPersonControl.character` in C++. We have the field offsets which makes this very easy.

```
void (*_Update)(void* _this);
void Update(void* _this)
{
    if (_this)
    {
        void* skinName = *(void **)((uint32_t)_this + 0x320); // internal SkinName mySkinName; // 0x320
        void* firstPersonControl = *(void **)((uint32_t)skinName + 0xD8); // public FirstPersonControlSharp firstPersonControl; // 0xD8
        void* characterController = *(void **)((uint32_t)firstPersonControl + 0x74); // internal CharacterController character; // 0x74
        // Now we have access to CharacterController.
        _Update(_this);
    }
}

MSHookFunction((void *) getRealOffset(offset), (void *) &Update, (void **) &_Update); // Player_move_c UpdateEffects   
```

Now we have access to CharacterController. In order to create NoClip we need to be able to set the CharacterController's radius. On line 80966 of [dump.cs](https://github.com/jbro129/Universal-Unity-NoClip/blob/main/il2cpp/Dump/dump.cs) we have `public void set_radius(float value); // RVA: 0xEAB82C Offset: 0xEAB82C`

We need to create a c++ function to be able to use it which is also easy.

`
void (*CharacterController_set_radius)(void* character, float radius) = (void (*)(void *, float ))getRealOffset(0xEAB82C); // CharacterController$$set_radius
`

In order to make the player clip, we will need to set the CharacterController's radius to infinity. Unity does not know how to handle a CharacterController with an infinite radius which results in your player clipping through all walls.

The C# code of this is `this.mySkinName.firstPersonControl.character.radius = INFINITY`

Lets put this all together.

```
void (*_Update)(void* _this);
void Update(void* _this)
{
    if (_this)
    {
        void* skinName = *(void **)((uint32_t)_this + 0x320); // internal SkinName mySkinName; // 0x320
        void* firstPersonControl = *(void **)((uint32_t)skinName + 0xD8); // public FirstPersonControlSharp firstPersonControl; // 0xD8
        void* characterController = *(void **)((uint32_t)firstPersonControl + 0x74); // internal CharacterController character; // 0x74

        void (*CharacterController_set_radius)(void* character, float radius) = (void (*)(void *, float ))getRealOffset(0xEAB82C); // CharacterController$$set_radius

        CharacterController_set_radius(characterController, INFINITY);
        
        _Update(_this);
    }
}


MSHookFunction((void *) getRealOffset(offset), (void *) &Update, (void **) &_Update); // Player_move_c UpdateEffects   
```

Now we have created NoClip. Obviously you do not want NoClip to be indefinite so lets add a way to revert back to normal.

```
bool toggleNoClip = false;
bool NoClip = false;

void (*_Update)(void* _this);
void Update(void* _this)
{
    if (_this)
    {
        void* skinName = *(void **)((uint32_t)_this + 0x320); // internal SkinName mySkinName; // 0x320
        void* firstPersonControl = *(void **)((uint32_t)skinName + 0xD8); // public FirstPersonControlSharp firstPersonControl; // 0xD8
        void* characterController = *(void **)((uint32_t)firstPersonControl + 0x74); // internal CharacterController character; // 0x74

        void (*CharacterController_set_radius)(void* character, float radius) = (void (*)(void *, float ))getRealOffset(0xEAB82C); // CharacterController$$set_radius

        if (toggleNoClip) // check to see if u have ever enabled no clip
        {	
            if (NoClip) // turn on NoClip
            {
                CharacterController_set_radius(characterController, INFINITY);
            }
            else // turn off NoClip
            {
                CharacterController_set_radius(characterController, 0.35f); // In order to get the original radius, you will have to get the value of the default radius. If a get_radius does no exist, then experiment around to find a good value. For PG3D I figured out that 0.35f is a good value.
            }
        }
        _Update(_this);
}


MSHookFunction((void *) getRealOffset(offset), (void *) &Update, (void **) &_Update); // Player_move_c UpdateEffects   
```

I am sure there are other better ways of implementing a way to go back and forth between NoClipping and regular clipping, this is just how I do it.

That is how to create NoClip in unity il2cpp. Obviously every game is different. I could've hooked into FirstPersonControlSharp.Update() and set the radius there, but that would require an extra hook, which I didn't want to do.

This way of implementing NoClip does not stop the game from checking to see if you are clipping, bypassing that would require something else and this tutorial isn't meant to provide a bypass.

As long as the game uses CharacterController to handle player movement, state, and collision then this should work, regardless of what classes or code is in between.

You can look at all file used for il2cpp [here](https://github.com/jbro129/Universal-Unity-NoClip/blob/main/il2cpp)

You can see this NoClip in effect in my YouTube video here:

[![Walk Through Walls In PG3D!! - Pixel Gun 3D NoClip Mod](https://img.youtube.com/vi/beXkuC7zr9M/hqdefault.jpg)](https://www.youtube.com/watch?v=beXkuC7zr9M)


## Mono

For this example I am going to be using PG3D v11.3.1. PG3D uses CharacterController to handle everything player related. **This example applies only to PG3D, other games may implement CharacterController differently.**

Similar to the il2cpp tutorial, we are going to be modifying the UpdateEffects method of the class Player_move_c.

The goal is to be able to access the games CharacterController from within the function we are going to be editing. Just like in the il2cpp section, we determined that PG3D's CharacterController can be accessed from within the Player_move_c class.

Navigate to the UpdateEffects function inside of the Player_move_c class inside of the DLL modification tool of your choice. [dnSpy](https://github.com/dnSpy/dnSpy) is recommended.

The function should look like this: 

![UpdateEffects original](/Previews/original.png)


Again, just like in the il2cpp section, we figured out that we can access PG3D's CharacterController via UpdateEffects -> mySkinName -> firstPersonControl -> character.

Accessing these fields in C# is much easier than how we did in the C++ hook.

```
this.mySkinName.firstPersonControl.character.radius = float.PositiveInfinity;
```

Use ```this``` to get the Player_move_c instance, use the ```mySkinName``` field to get the games SkinName instance, use the ```firstPersonControl``` field to get the games FirstPersonControlSharp instance, then use the ```character``` field to get the games CharacterController instance.

From there we can access the CharacterController's radius method and use the setter to set the radius value to ```float.PositiveInfinity```.

![UpdateEffects modified](/Previews/modified.png)

Save your changes, save the DLL, and use the modified DLL inside your mod. (Your player will be in a constant state of NoClip so you might want to do something about that)

Just like that we have achieved NoClip in mono. 

Big thanks to [ArcyMods](https://github.com/ArcyMods) for helping and testing and verifying this mono method.

*Preview in progress*


