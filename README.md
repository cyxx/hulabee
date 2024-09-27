
# Hulabee Entertainment Games Tools and VM

![Hulabee Logo](hulabee_400.png)

[Hulabee Entertainment](https://www.mobygames.com/company/5605/hulabee-entertainment-inc/) developed a proprietary engine initially for its adventure kid games.

The internal name was probably 'Sauce' as referenced in the executable.

```
$ strings MoopTreasure.exe | grep -i engine
ESauce Engine Error: %s
name="Hulabee.ESauce.Engine"
```

This repository contains:

* a [rewrite of the engine](vm/README.md) using SDL2
* a version of [whisk.dll](https://www.dropbox.com/s/pey60eh8rnqftnm/whisk.dll?dl=0) to enable VM opcodes tracing
* some tools to extract and convert assets from `.pan` and `.gg` files
* some miscellaneous [information](docs/information.md) about the engine and the games
