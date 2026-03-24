# Music Randomization
Music randomization works by setting the music volume to 0 in game and then monitoring what track its currently playing. From there we take the internal track name and look for a folder in the music directory with the same name and randomly select an mp3 from it to play. Below is a list of the folder names and the title of their original song from Final Fantasy 7. If you do not include the original track in the folder it will not be an option to play, which means you can replace the entire soundtrack if you wish.

# Music Folders
| Folder Name | Song |
| ---- | --- |
| aseri | Hurry! |
| aseri2 | Hurry, Faster! |
| ayasi | Lurking in the Darkness |
| barret | Barret's Theme |
| bat | Fighting |
| bee | Honeybee Manor |
| bokujo | Farm Boy |
| boo | Life Stream |
| cannon | The Mako Cannon Fires! |
| canyon | Cosmo Canyon |
| cephiros | Those Chosen By the Planet |
| chase | Crazy Motorcycle Chase |
| chu | Still More Fighting |
| chu2 | J-E-N-O-V-A |
| cinco | Cinco de Chocobo |
| cintro | Those Chosen By the Planet |
| comical | Comical (deleted song) |
| condor | Fortress of the Condor |
| corel | Mining Town |
| corneo | Don of the Slums |
| costa | Costa del Sol |
| crlost | Tango of Tears |
| crwin | A Great Success |
| date | Interrupted by Fireworks |
| dokubo | Underneath the Rotting Pizza |
| dun2 | Chasing the Black-Caped Man |
| earis | Aeris' Theme |
| earislo | Flowers Blooming in the Church |
| elec | Electric de Chocobo |
| fan2 | Fanfare |
| fanfare | Fanfare |
| fiddle | Fiddle de Chocobo |
| fin | World Crisis |
| geki | Debut |
| gold1 | Gold Saucer |
| guitar2 | On the Other Side of the Mountain |
| gun | Full-Scale Attack |
| hen | Who Am I |
| hiku | The Highwind Takes to the Skies |
| horror | Trail of Blood |
| iseki | You Can Hear the Cry of the Planet |
| jukai | Forested Temple |
| junon | Off the Edge of Despair |
| jyro | Steal the Tiny Bronco! |
| ketc | Cait Sith's Theme |
| kita | The Great Northern Cave |
| kurai | Anxious Heart |
| lb1 | The Birth of God |
| lb2 | One-Winged Angel |
| ld | Judgement Day |
| makoro | Mako Reactor |
| mati | Ahead on Our Way |
| mekyu | Reunion |
| mogu | The Highwind Takes to the Skies (Moggle version) |
| mura1 | Parochial Town |
| nointro | Shinra Corporation |
| oa | Opening ~ Bombing Mission |
| ob | Bombing Mission |
| odds | Cinco de Chocobo |
| over2 | Continue |
| parade | Rufus' Welcoming Ceremony |
| pj | Jenova Absolute |
| pre | The Prelude |
| red | Red XIII's Theme |
| rhythm | Turks' Theme |
| riku | Reunion |
| ro | The Countdown Begins |
| rocket | Oppressed People |
| roll | Staff Roll |
| rukei | Sandy Badlands |
| sadbar | Mark of the Traitor |
| sadsid | Sending a Dream into the Universe |
| sea | A Secret Sleeping in the Deep Sea |
| seto | Great Warrior |
| si | Reunion |
| sid2 | Cid's Theme |
| sido | It's Difficult to Stand on Both Feet, Isn't It |
| siera | If You Open Your Heart... |
| sinra | Shinra Corporation |
| sinraslo | Infiltrating Shinra Tower |
| snow | Buried in the Snow |
| snowboarding | See note below |
| ta | Main Theme of Final Fantasy VII |
| tb | Main Theme of Final Fantasy VII (alternate) |
| tender | Holding My Thoughts in My Heart |
| tifa | Tifa's Theme |
| tm | On That Day, 5 Years Ago |
| utai | Wutai |
| vincent | The Nightmare's Beginning |
| walz | Waltz de Chocobo |
| weapon | Weapon Raid |
| wind | Wind |
| yado | Good Night, Until Tomorrow! |
| yufi | Descendant of Shinobi |
| yufi2 | Stolen Materia |
| yume | Who Are You |

# Snowboarding Folder
The game normally does not play music during the snowboarding section at the beginning of Disc 2, however, it is common for streamers to play music during this section in its absense. To accomodate this support for a `snowboarding` folder was added that can be filled with songs that can be randomly selected for the snowboarding section.

# cfg Files
Some tracks have an intro and then loop a subsection of the song so music randomization includes the ability to do this as well. If you create a .cfg file which has the same name as an mp3 in the folder, when the mp3 is loaded the config file will be loaded with it and used when the song is selected to play. 

| Parameter Name | Description | Example |
| --- | ----- | --- |
| Start | Where the song should begin playback from | Start=1000000 |
| LoopStart | Where the looping section begins | LoopStart=2447550 |
| LoopEnd | Wehre the looping section ends | LoopEnd=11555543 |
| NoFade | Prevents the song from fading in | NoFade=true |
| NoLoop | Prevents the song from looping | NoLoop=true |

Note: Start, LoopStart, and LoopEnd are expressed in PCM samples. Music is assumed to be at 48 kHz sample rate.  
