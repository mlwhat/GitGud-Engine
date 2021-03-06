[![Build status](https://ci.appveyor.com/api/projects/status/83to2wdrv5p7kmt8?svg=true)](https://ci.appveyor.com/project/Josef212/gitgud-engine)

<h1>GitGud-Engine</h1>

<p>GitGud is an OpenGL engine done for educational purposes as a continuation of <a href="https://github.com/Josef212/JayEngine">JayEngine</a> done for the game engine assignement.</p>
<p>The engine is still in a really early state but will be working on my free time.</p>

<ul>
	<li><a href="https://github.com/Josef212/GitGud-Engine">Repository</a></li>
	<li><a href="https://github.com/Josef212">Github</a></li>
	<li><a href="https://github.com/Josef212/GitGud-Engine/issues">Report an issue</a></li>
	<li><a href="https://github.com/Josef212/GitGud-Engine/releases">Releases</a></li>
</ul>

<h2>CHANGELOG</h2>

<h3>v0.1.7</h3>
<p>Some shader progress.</p>
<ul>
	<li>Default used hardcoded and given by a resource.</li>
	<li>Can create shaders from MainMenu->Resources->ShaderEditor => File->CreateNew. The set the name and create.</li>
	<li>Edit shaders with extern tools, shaders files are located under Data/Library/Shaders/name.vertex-fragment</li>
	<li>Shaders are compiled on engine load or forced from Shader editor File->Force compile</li>
	<li>Shader text is displayed, for now can't be edited from the engine.</li>
	<li>Shaders will be only used when materials are implemented.</li>
	<li>Also added gpu info displayed in Configuration->Information window.</li> 
</ul>

<h3>v0.1.6</h3>
<p>Small progress.</p>
<ul>
	<li>Shader prints vertex normal as color.</li>
	<li>Scene load and save. Unique scene for now, build comes with a default scene (File->Load scene).</li>
	<li>Using a simple render pipeline to render game objects.</li>
	<li>Fixed an issue with transform component.</li>
</ul>

<h3>v0.1.5</h3>
<p>First engine version, basic structure is created to build the engine on it.</p>
<p>Current features:</p>
<ul>
<li>Default shader loaded and a cube displayed in origin. This is just for test so is not linked to game object system.</li>
<li>Basic game object system with components. Hierarchy and inspector display objects information.</li>
<li>Editor camera. Move it using WASD, RF and right click + drag to rotate.(Rotating is still a bit bad but working on it)</li>
<li>Scene(fbx, obj, ...) and texture importers to own formats.</li>
<li>Scene save and load. Still not full functionality but can serialize current game objects and load them later.</li>
<li>Simple console. Type "help" to display all commands.</li>
<li>Some app configuration.</li>
<li>UI style editor.</li>
</ul>


