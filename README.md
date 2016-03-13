# BlocklandLoader

This DLL will hook `Sim::init` and load all DLLs found in the `Blockland/modules/` folder for you. It provides a dummy symbol named `loader` that you can add to the import table for automatic loading of this DLL during Blockland startup.

### User instructions

Create a folder named `modules` (in your Blockland folder) and put all your DLLs in it.

##### From a release

Put `Blockland.exe` and `BlocklandLoader.dll` in your Blockland folder.


##### Doing it yourself

Build the `BlocklandLoader` project in Visual Studio, and copy the output file `BlocklandLoader.dll` to your Blockland folder.

For patching the import table, I recommend [StudPE](http://www.cgsoftlabs.ro/studpe.html).

1. Open `Blockland.exe` (File -> Open PE File)
2. Go to the **Functions** tab
3. Right click in the left panel and click **Add New Import**.
4. Click **Dll Select** and browse to `BlocklandLoader.dll` in your Blockland folder.
5. Click **Select func.** and select `loader` from the list that appears.
6. Click `Add to list`, then `ADD` at the bottom.
7. Close the program by clicking `OK`.

### Developer instructions

BlocklandLoader will load your DLL using `LoadLibraryA` immediately before `Sim::init` in the main execution thread of `Blockland.exe`. You can perform your initialization in `DllMain`. This loader does not yet provide any addresses or hooks for you, you'll need to do that in your DLL yourself.
