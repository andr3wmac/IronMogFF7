import glob
import os
import requests
import shutil
import subprocess
import time
import zipfile
from mutagen.easyid3 import EasyID3
from pathlib import Path
from urllib.parse import unquote

download_music   = True
create_folders   = True
unzip_sources    = True
delete_unwanted  = True
rename_files     = True
normalize_volume = True
populate_music   = True

def deleteFile(path):
    if os.path.exists(path):
        os.remove(path)

def deleteFolder(path):
    if os.path.exists(path):
        shutil.rmtree(path)

def downloadFile(url):
    script_dir = Path(__file__).parent
    filename = unquote(url.split("/")[-1])
    save_path = script_dir / "workspace" / filename

    if os.path.exists(save_path):
        return False

    response = requests.get(url, stream=True)
    response.raise_for_status()

    with open(save_path, 'wb') as file:
        for chunk in response.iter_content(chunk_size=8192):
            file.write(chunk)
            
    print(f"File saved to: {save_path}")
    return True

if download_music:
    print("Downloading Final Fantasy VI music..")
    if downloadFile("https://fi.zophar.net/soundfiles/playstation-psf/final-fantasy-vi/ulyhpsqq/Final%20Fantasy%20VI%20%28MP3%29.zophar.zip"):
        time.sleep(10)
    else:
        print("Final Fantasy VI music already downloaded, skipping..")

    print("Downloading Final Fantasy VII music..")
    if downloadFile("https://fi.zophar.net/soundfiles/playstation-psf/final-fantasy-vii/uoyddwhr/Final%20Fantasy%20VII%20%28MP3%29.zophar.zip"):
        time.sleep(10)
    else:
        print("Final Fantasy VII music already downloaded, skipping..")

    print("Downloading Final Fantasy VIII music..")
    if downloadFile("https://fi.zophar.net/soundfiles/playstation-psf/final-fantasy-viii/ctseevll/Final%20Fantasy%20VIII%20%28MP3%29.zophar.zip"):
        time.sleep(10)
    else:
        print("Final Fantasy VIII music already downloaded, skipping..")

    print("Downloading Final Fantasy IX music..")
    if not downloadFile("https://fi.zophar.net/soundfiles/playstation-psf/final-fantasy-ix/iseszzgf/Final%20Fantasy%20IX%20%28MP3%29.zophar.zip"):
        print("Final Fantasy IX music already downloaded, skipping..")

if create_folders:
    deleteFolder("music")

    print("Creating folders..")
    music_list = [
        "none", "nothing", "oa", "ob", "dun2", "guitar2", "fanfare", "makoro", "bat",
        "fiddle", "kurai", "chu", "ketc", "earis", "ta", "tb", "sato",
        "parade", "comical", "yume", "mati", "sido", "siera", "walz", "corneo",
        "horror", "canyon", "red", "seto", "ayasi", "sinra", "sinraslo", "dokubo",
        "bokujo", "tm", "tifa", "costa", "rocket", "earislo", "chase", "rukei",
        "cephiros", "barret", "corel", "boo", "elec", "rhythm", "fan2", "hiku",
        "cannon", "date", "cintro", "cinco", "chu2", "yufi", "aseri", "gold1",
        "mura1", "yado", "over2", "crwin", "crlost", "odds", "geki", "junon",
        "tender", "wind", "vincent", "bee", "jukai", "sadbar", "aseri2", "kita",
        "sid2", "sadsid", "iseki", "hen", "utai", "snow", "yufi2", "mekyu",
        "condor", "lb2", "gun", "weapon", "pj", "sea", "ld", "lb1",
        "sensui", "ro", "jyro", "nointro", "riku", "si", "mogu", "pre",
        "fin", "heart", "roll"
    ]

    # Create folders for all entries except 'none' and 'nothing'
    for name in music_list:
        if name in ("none", "nothing"):
            continue
        os.makedirs("music/" + name, exist_ok=True)

# Unzip the source files
if unzip_sources:
    print("Unzipping music..")
    deleteFolder("workspace/FF6/")
    os.makedirs("workspace/FF6/", exist_ok=True)
    with zipfile.ZipFile("workspace/Final Fantasy VI (MP3).zophar.zip", 'r') as zip_ref:
        zip_ref.extractall("workspace/FF6/")

    deleteFolder("workspace/FF7/")
    os.makedirs("workspace/FF7/", exist_ok=True)
    with zipfile.ZipFile("workspace/Final Fantasy VII (MP3).zophar.zip", 'r') as zip_ref:
        zip_ref.extractall("workspace/FF7/")

    deleteFolder("workspace/FF8/")
    os.makedirs("workspace/FF8/", exist_ok=True)
    with zipfile.ZipFile("workspace/Final Fantasy VIII (MP3).zophar.zip", 'r') as zip_ref:
        zip_ref.extractall("workspace/FF8/")

    deleteFolder("workspace/FF9/")
    os.makedirs("workspace/FF9/", exist_ok=True)
    with zipfile.ZipFile("workspace/Final Fantasy IX (MP3).zophar.zip", 'r') as zip_ref:
        zip_ref.extractall("workspace/FF9/")

if delete_unwanted:
    print("Deleting unwanted files..")

    deleteFile("workspace/FF6/999 The Empire Gestahl (alternate1).mp3")
    deleteFile("workspace/FF6/999 The Empire Gestahl (alternate2).mp3")
    deleteFile("workspace/FF6/999 The Empire Gestahl (alternate3).mp3")
    deleteFile("workspace/FF6/999 The Empire Gestahl (alternate4).mp3")
    deleteFile("workspace/FF6/999 The Empire Gestahl (alternate5).mp3")
    deleteFile("workspace/FF6/999 The Empire Gestahl (alternate6).mp3")
    deleteFile("workspace/FF6/999 Unknown 1e.mp3")
    deleteFile("workspace/FF6/999 Unknown 3a.mp3")
    deleteFile("workspace/FF6/999 Unknown 3f.mp3")
    deleteFile("workspace/FF6/999 Unknown 4a.mp3")
    deleteFile("workspace/FF6/999 Unknown 20.mp3")
    deleteFile("workspace/FF6/999 Unknown 39.mp3")
    deleteFile("workspace/FF6/999 Unknown 40.mp3")
    deleteFile("workspace/FF6/999 Unknown 48.mp3")
    deleteFile("workspace/FF6/999 Unknown 49.mp3")
    deleteFile("workspace/FF6/SLPM-86198_END_0000408c.mp3")
    deleteFile("workspace/FF6/SLPM-86198_F6XBG00_0000096c.mp3")
    deleteFile("workspace/FF6/SLPM-86198_OPEN_0000408c.mp3")
    deleteFile("workspace/FF6/SLPM-86198_OPERA_0000408c.mp3")

    deleteFile("workspace/FF9/424 CCJC TVCM 15' (Coca-Cola Commercial).mp3")
    deleteFile("workspace/FF9/425 CCJC TVCM 30' (Coca-Cola Commercial).mp3")

if rename_files:
    print("Renaming files..")
    def renameToTitles(folder, prefix):
        for root, _, files in os.walk(folder):
            for file in files:
                if file.lower().endswith(".mp3"):
                    path = os.path.join(root, file)
                    try:
                        tags = EasyID3(path)
                        title = tags.get("title", ["<No Title>"])[0]
                    except Exception:
                        title = "<No Title>"

                    if title == "??":
                        title = "Unknown"

                    title = title.replace("?", "")
                    os.rename(path, folder + prefix + " - " + title + ".mp3")
    
    renameToTitles("workspace/FF6/", "FF6")
    renameToTitles("workspace/FF7/", "FF7")
    renameToTitles("workspace/FF8/", "FF8")
    renameToTitles("workspace/FF9/", "FF9")

def batch_normalize_folder(input_dir, output_dir, target_lufs=-14):
    # Get all mp3 files
    files = glob.glob(f"{input_dir}/*.mp3")
    
    command = [
        "ffmpeg-normalize",
        *files,
        "--preset", "podcast",
        "-c:a", "libmp3lame",
        "-b:a", "320k",
        "-of", output_dir,
        "-ext", "mp3",
        "-f"
    ]
    
    subprocess.run(command)

if normalize_volume:
    print("Normalizing Volume..")
    batch_normalize_folder("workspace/FF6/", "workspace/FF6_Normalized/")
    batch_normalize_folder("workspace/FF7/", "workspace/FF7_Normalized/")
    batch_normalize_folder("workspace/FF8/", "workspace/FF8_Normalized/")
    batch_normalize_folder("workspace/FF9/", "workspace/FF9_Normalized/")

if populate_music:
    print("Populating music folder..")
    music = {}
    music["aseri"] = ['FF7 - Hurry!.mp3', "FF8 - Movin'.mp3", 'FF8 - Never Look Back.mp3', 'FF8 - The Mission.mp3', 'FF8 - The Stage is Set.mp3', 'FF9 - Ambush Attack.mp3', 'FF9 - Assault of the White Dragons.mp3', 'FF9 - Feel My Blade.mp3', "FF9 - Hunter's Chance.mp3", 'FF9 - Run!.mp3']
    music["aseri2"] = ['FF7 - Hurry, Faster!.mp3', 'FF8 - Never Look Back.mp3', 'FF9 - Ambush Attack.mp3', 'FF9 - Assault of the White Dragons.mp3', 'FF9 - Run!.mp3']
    music["ayasi"] = ['FF7 - Lurking in the Darkness.mp3', 'FF8 - Fear.mp3', 'FF8 - Intruders.mp3', 'FF8 - The Spy.mp3']
    music["barret"] = ["FF7 - Barret's Theme.mp3"]
    music["bat"] = ['FF6 - Battle Theme.mp3', 'FF7 - Fighting.mp3', "FF8 - Don't Be Afraid.mp3", 'FF8 - Force Your Way.mp3', 'FF8 - The Man With the Machine Gun.mp3', 'FF9 - Battle.mp3']
    music["bee"] = ['FF6 - The Magic House.mp3', 'FF7 - Honeybee Manor.mp3', "FF9 - Quina's Theme.mp3"]
    music["bokujo"] = ['FF7 - Farm Boy.mp3']
    music["boo"] = ['FF7 - Life Stream.mp3']
    music["cannon"] = ['FF6 - Catastrophe.mp3', 'FF6 - Metamorphasis.mp3', 'FF7 - The Mako Cannon Fires!.mp3', 'FF8 - Dead End.mp3', 'FF8 - Retaliation.mp3', 'FF9 - Assault of the White Dragons.mp3']
    music["canyon"] = ['FF6 - Cyan.mp3', 'FF6 - Shadow.mp3', 'FF7 - Cosmo Canyon.mp3', 'FF9 - Mountain Pass.mp3', 'FF9 - Oeilvert.mp3']
    music["cephiros"] = ['FF7 - Those Chosen By the Planet.mp3', 'FF8 - Succession of Witches.mp3', 'FF9 - Iifa Tree.mp3']
    music["chase"] = ['FF7 - Crazy Motorcycle Chase.mp3']
    music["chu"] = ['FF6 - The Decisive Battle.mp3', 'FF7 - Still More Fighting.mp3', 'FF8 - Force Your Way.mp3', 'FF9 - Boss Battle.mp3']
    music["chu2"] = ['FF7 - J-E-N-O-V-A.mp3']
    music["cinco"] = ['FF7 - Cinco de Chocobo.mp3']
    music["cintro"] = ['FF6 - The Empire Gestahl.mp3', 'FF7 - Those Chosen By the Planet.mp3', 'FF8 - Fithos Lusec Wecos Vinosec.mp3', 'FF9 - Keeper of Time.mp3']
    music["comical"] = []
    music["condor"] = ['FF6 - Troops March on.mp3', 'FF7 - Fortress of the Condor.mp3']
    music["corel"] = ['FF7 - Mining Town.mp3', 'FF9 - Ceremony for the Gods.mp3', 'FF9 - Ruins of Madain Sari.mp3']
    music["corneo"] = ['FF6 - Gogo.mp3', 'FF6 - Slam Shuffle.mp3', 'FF7 - Don of the Slums.mp3', 'FF8 - Residents.mp3', 'FF8 - Silence and Motion.mp3', 'FF9 - Gargan Roo.mp3', 'FF9 - Jesters of the Moon.mp3', 'FF9 - The City that Never Sleeps.mp3', 'FF9 - The Four Medallions.mp3', 'FF9 - Theme of Tantalus.mp3']
    music["costa"] = ['FF7 - Costa del Sol.mp3']
    music["crlost"] = ['FF7 - Tango of Tears.mp3']
    music["crwin"] = ['FF7 - A Great Success.mp3']
    music["date"] = ['FF6 - Celes.mp3', 'FF6 - Relm.mp3', 'FF7 - Interrupted by Fireworks.mp3', 'FF8 - Ami.mp3', 'FF8 - Tell Me.mp3', 'FF8 - Where I Belong.mp3', 'FF9 - Memories Erased in the Storm.mp3', 'FF9 - Stolen Eyes.mp3', 'FF9 - The Heart of Melting Magic.mp3']    
    music["dokubo"] = ['FF7 - Underneath the Rotting Pizza.mp3', 'FF8 - Galbadia Garden (No Intro).mp3', 'FF8 - Jailed.mp3', 'FF8 - Martial Law.mp3', 'FF8 - Under Her Control.mp3', 'FF9 - Lindblum.mp3', "FF9 - Vivi's Theme.mp3", 'FF9 - We Are Thieves!.mp3']
    music["dun2"] = ['FF6 - Another World of Beasts.mp3', 'FF7 - Chasing the Black-Caped Man.mp3', 'FF8 - Compression of Time.mp3', 'FF8 - Junction.mp3', 'FF8 - Lunatic Pandora.mp3', 'FF9 - A Transient Past.mp3', 'FF9 - Black Waltz.mp3', "FF9 - Cleyra's Trunk.mp3", 'FF9 - Footsteps of Desire.mp3', 'FF9 - Memories of That Day.mp3', 'FF9 - The Chosen Summoner.mp3']
    music["earis"] = ['FF6 - Aria de Mezzo Carattere.mp3', "FF7 - Aeris' Theme.mp3", 'FF8 - Fragments of Memories.mp3']
    music["earislo"] = ['FF7 - Flowers Blooming in the Church.mp3']
    music["elec"] = ['FF6 - Techno de Chocobo.mp3', 'FF7 - Electric de Chocobo.mp3', 'FF8 - Mods de Chocobo.mp3']
    music["fan2"] = ['FF6 - Fanfare.mp3', 'FF7 - Fanfare.mp3', 'FF8 - The Winner.mp3', 'FF9 - Fanfare.mp3']
    music["fanfare"] = ['FF6 - Fanfare.mp3', 'FF7 - Fanfare.mp3', 'FF8 - The Winner.mp3', 'FF9 - Fanfare.mp3']
    music["fiddle"] = ['FF7 - Fiddle de Chocobo.mp3']
    music["fin"] = ['FF7 - World Crisis.mp3']
    music["geki"] = ['FF6 - Kefka.mp3', 'FF6 - Mog.mp3', 'FF6 - Spinach Rag.mp3', 'FF7 - Debut.mp3', 'FF8 - Slide Show Part 1.mp3', "FF9 - Moogle's Theme.mp3", 'FF9 - Slew of Love Letters.mp3', 'FF9 - The Sneaky Frog and the Scoundrel.mp3']
    music["gold1"] = ['FF6 - Johnny C. Bad.mp3', 'FF7 - Gold Saucer.mp3', 'FF8 - Shuffle or Boogie.mp3', "FF9 - Black Mage's Village.mp3", 'FF9 - Jesters of the Moon.mp3', 'FF9 - Prima Vista Band.mp3']
    music["guitar2"] = ['FF6 - Coin Song.mp3', 'FF6 - Forever Rachel!.mp3', 'FF7 - On the Other Side of the Mountain.mp3', 'FF8 - Julia.mp3', 'FF8 - Roses and Wine.mp3', 'FF9 - Dissipating Sorrow.mp3', 'FF9 - Song of Memories.mp3']
    music["gun"] = ['FF7 - Full-Scale Attack.mp3', 'FF8 - Never Look Back.mp3', 'FF8 - The Mission.mp3', 'FF8 - The Stage is Set.mp3', 'FF9 - Ambush Attack.mp3', 'FF9 - Assault of the White Dragons.mp3', "FF9 - Hunter's Chance.mp3"]
    music["hen"] = ['FF6 - Another World of Beasts.mp3', 'FF7 - Who Am I.mp3', 'FF8 - Compression of Time.mp3', 'FF8 - Find Your Way.mp3', 'FF8 - Junction.mp3', 'FF8 - Lunatic Pandora.mp3', 'FF9 - A Transient Past.mp3', 'FF9 - Black Waltz.mp3', "FF9 - Cleyra's Trunk.mp3", 'FF9 - Extraction.mp3', 'FF9 - Footsteps of Desire.mp3', 'FF9 - The Chosen Summoner.mp3']
    music["hiku"] = ['FF6 - Blackjack.mp3', 'FF6 - Searching for Friends.mp3', 'FF7 - The Highwind Takes to the Skies.mp3', 'FF8 - Ride On.mp3', 'FF9 - The Airship, Hilda Garde.mp3']
    music["horror"] = ['FF7 - Trail of Blood.mp3', 'FF8 - Rivals.mp3']
    music["iseki"] = ['FF7 - You Can Hear the Cry of the Planet.mp3', "FF9 - Ipsen's Castle.mp3", 'FF9 - Rebirth of the Evil Mist.mp3', 'FF9 - Terra.mp3', 'FF9 - Wall of Sacred Beasts.mp3']
    music["jukai"] = ['FF6 - The Phantom Forest.mp3', 'FF7 - Forested Temple.mp3', 'FF9 - Awakening the Forest.mp3', 'FF9 - Place of Memory.mp3']
    music["junon"] = ['FF6 - Another World of Beasts.mp3', 'FF7 - Off the Edge of Despair.mp3', 'FF8 - Compression of Time.mp3', 'FF8 - Junction.mp3', 'FF8 - Lunatic Pandora.mp3', 'FF9 - A Transient Past.mp3', 'FF9 - Battle Strategy Conference.mp3', "FF9 - Cleyra's Trunk.mp3", 'FF9 - Extraction.mp3', 'FF9 - Footsteps of Desire.mp3', 'FF9 - The Chosen Summoner.mp3']
    music["jyro"] = ['FF6 - Setzer.mp3', 'FF7 - Steal the Tiny Bronco!.mp3', "FF9 - You're Not Alone!.mp3", "FF9 - Zidane's Theme.mp3"]
    music["ketc"] = ["FF7 - Cait Sith's Theme.mp3"]
    music["kita"] = ['FF6 - The Empire Gestahl.mp3', 'FF7 - The Great Northern Cave.mp3']
    music["kurai"] = ['FF6 - Awakening.mp3', 'FF6 - Forever Rachel!.mp3', 'FF6 - The Mines of Narshe.mp3', 'FF7 - Anxious Heart.mp3', 'FF8 - Galbadia Garden (No Intro).mp3', 'FF9 - Soulless Village.mp3']
    music["lb1"] = ['FF7 - The Birth of God.mp3', 'FF9 - Dark Messenger.mp3']
    music["lb2"] = ['FF6 - Dancing Mad (Part1).mp3', 'FF7 - One-Winged Angel.mp3', "FF8 - Maybe I'm a Lion.mp3", 'FF9 - Final Battle.mp3']
    music["ld"] = ['FF7 - Judgement Day.mp3', "FF9 - Cid's Theme.mp3", 'FF9 - Kingdom of Burmecia.mp3']
    music["makoro"] = ['FF6 - Troops March on.mp3', 'FF7 - Mako Reactor.mp3', 'FF8 - Only a Plank Between One and Perdition.mp3']
    music["mati"] = ['FF6 - Epitaph.mp3', 'FF6 - Gau.mp3', 'FF6 - Kids Run Through the City.mp3', 'FF7 - Ahead on Our Way.mp3', 'FF8 - Balamb Garden.mp3', 'FF8 - Breezy.mp3', "FF8 - Fisherman's Horizon.mp3", 'FF8 - Fragments of Memories.mp3', 'FF8 - Love Grows.mp3', 'FF9 - At the South Gate Border.mp3', 'FF9 - Frontier Village Dali.mp3', "FF9 - Garnet's Theme.mp3", 'FF9 - Secret Library Daguerreo.mp3']
    music["mekyu"] = ['FF7 - Reunion.mp3']
    music["mogu"] = ['FF7 - The Highwind Takes to the Skies.mp3', 'FF8 - Ride On.mp3', 'FF9 - The Airship, Hilda Garde.mp3']
    music["mura1"] = ['FF7 - Parochial Town.mp3', 'FF8 - Galbadia Garden (No Intro).mp3', 'FF8 - Jailed.mp3', 'FF8 - Martial Law.mp3', 'FF8 - Under Her Control.mp3', 'FF9 - Lindblum.mp3', "FF9 - Vivi's Theme.mp3", 'FF9 - We Are Thieves!.mp3']
    music["nointro"] = ['FF7 - Shinra Corporation.mp3']
    music["oa"] = ['FF7 - Opening ~ Bombing Mission.mp3', 'FF8 - The Landing.mp3']
    music["ob"] = ['FF7 - Bombing Mission.mp3', 'FF8 - The Landing.mp3']
    music["odds"] = ['FF7 - Cinco de Chocobo.mp3']
    music["over2"] = ['FF6 - Rest in Peace.mp3', 'FF7 - Continue.mp3', 'FF8 - The Loser.mp3', 'FF9 - Game Over.mp3']
    music["parade"] = ["FF7 - Rufus' Welcoming Ceremony.mp3", 'FF8 - Cactus Jack (Galbadian Anthem).mp3']
    music["pj"] = ['FF6 - The Fierce Battle.mp3', 'FF6 - The Unforgiven.mp3', 'FF7 - Jenova Absolute.mp3', 'FF8 - Premonition.mp3', 'FF8 - The Legendary Beast.mp3']
    music["pre"] = ['FF7 - The Prelude.mp3', 'FF9 - Prelude.mp3']
    music["red"] = ['FF6 - The Day After.mp3', 'FF6 - Wild West.mp3', "FF7 - Red XIII's Theme.mp3", 'FF9 - Mountain Pass.mp3']
    music["rhythm"] = ["FF7 - Turks' Theme.mp3", 'FF8 - The Spy.mp3', "FF9 - Amarant's Theme.mp3"]
    music["riku"] = ['FF7 - Reunion.mp3']
    music["ro"] = ['FF7 - The Countdown Begins.mp3', 'FF8 - Dead End.mp3', 'FF8 - Starting Up.mp3']
    music["rocket"] = ['FF7 - Oppressed People.mp3', 'FF8 - Galbadia Garden (No Intro).mp3', 'FF8 - Jailed.mp3', 'FF8 - Martial Law.mp3', 'FF8 - Under Her Control.mp3', 'FF9 - Lindblum.mp3', "FF9 - Vivi's Theme.mp3", 'FF9 - We Are Thieves!.mp3']
    music["roll"] = ['FF6 - Ending Theme (Part2).mp3', 'FF7 - Staff Roll.mp3', 'FF8 - Overture.mp3']
    music["rukei"] = ['FF7 - Sandy Badlands.mp3', 'FF9 - Ceremony for the Gods.mp3', 'FF9 - Ruins of Madain Sari.mp3']
    music["sadbar"] = ['FF7 - Mark of the Traitor.mp3', "FF9 - Amarant's Theme.mp3", 'FF9 - Ceremony for the Gods.mp3', 'FF9 - Ruins of Madain Sari.mp3']
    music["sadsid"] = ['FF6 - Coin Song.mp3', 'FF7 - Sending a Dream into the Universe.mp3', 'FF8 - My Mind.mp3']
    music["sea"] = ['FF6 - The Serpent Trench.mp3', 'FF7 - A Secret Sleeping in the Deep Sea.mp3']
    music["seto"] = ['FF7 - Great Warrior.mp3', 'FF9 - Dissipating Sorrow.mp3', 'FF9 - Forgotten Face.mp3']
    music["si"] = ['FF6 - Another World of Beasts.mp3', 'FF7 - Reunion.mp3', 'FF8 - Compression of Time.mp3', 'FF8 - Drifting.mp3', 'FF8 - Junction.mp3', 'FF8 - Lunatic Pandora.mp3', 'FF9 - A Transient Past.mp3', 'FF9 - Black Waltz.mp3', "FF9 - Cleyra's Trunk.mp3", 'FF9 - Extraction.mp3', 'FF9 - Footsteps of Desire.mp3', 'FF9 - Memories of That Day.mp3', 'FF9 - The Chosen Summoner.mp3']
    music["sid2"] = ["FF7 - Cid's Theme.mp3"]
    music["sido"] = ["FF7 - It's Difficult to Stand on Both Feet, Isn't It.mp3", 'FF8 - Residents.mp3', 'FF9 - Gargan Roo.mp3', 'FF9 - Jesters of the Moon.mp3', 'FF9 - The Four Medallions.mp3', 'FF9 - Theme of Tantalus.mp3']
    music["siera"] = ['FF7 - If You Open Your Heart....mp3', 'FF8 - Trust Me.mp3']
    music["sinra"] = ['FF6 - The Empire Gestahl.mp3', 'FF7 - Shinra Corporation.mp3', 'FF8 - A Sacrifice.mp3', "FF9 - Kuja's Theme.mp3"]
    music["sinraslo"] = ['FF6 - Under Martial Law.mp3', 'FF7 - Infiltrating Shinra Tower.mp3', 'FF8 - Only a Plank Between One and Perdition.mp3', 'FF9 - Kingdom of Burmecia.mp3']
    music["snow"] = ['FF7 - Buried in the Snow.mp3', 'FF9 - Esto Gaza.mp3']
    music["ta"] = ['FF6 - Searching for Friends.mp3', 'FF6 - Terra.mp3', 'FF7 - Main Theme of Final Fantasy VII.mp3', 'FF8 - Blue Fields.mp3', 'FF9 - Crossing Those Hills.mp3']
    music["tb"] = ['FF7 - Main Theme of Final Fantasy VII.mp3']
    music["tender"] = ['FF6 - Relm.mp3', 'FF7 - Holding My Thoughts in My Heart.mp3', 'FF8 - Ami.mp3', 'FF8 - Tell Me.mp3']
    music["tifa"] = ["FF7 - Tifa's Theme.mp3"]
    music["tm"] = ['FF7 - On That Day, 5 Years Ago.mp3', 'FF8 - Drifting.mp3', 'FF8 - Unrest.mp3', 'FF9 - Black Waltz.mp3', 'FF9 - Footsteps of Desire.mp3', 'FF9 - The Chosen Summoner.mp3']
    music["utai"] = ['FF7 - Wutai.mp3', 'FF9 - Oeilvert.mp3']
    music["vincent"] = ['FF6 - Another World of Beasts.mp3', "FF7 - The Nightmare's Beginning.mp3", 'FF8 - Lunatic Pandora.mp3', 'FF9 - Memories of That Day.mp3', 'FF9 - The Chosen Summoner.mp3']
    music["walz"] = ['FF7 - Waltz de Chocobo.mp3', 'FF9 - Ukulele de Chocobo.mp3']
    music["weapon"] = ['FF7 - Weapon Raid.mp3', 'FF8 - Only a Plank Between One and Perdition.mp3', 'FF9 - Assault of the White Dragons.mp3']
    music["yado"] = ['FF7 - Good Night, Until Tomorrow!.mp3', 'FF9 - Goodnight.mp3']
    music["yufi"] = ['FF7 - Descendant of Shinobi.mp3']
    music["yufi2"] = ['FF7 - Stolen Materia.mp3', 'FF9 - Gargan Roo.mp3', 'FF9 - Jesters of the Moon.mp3', 'FF9 - The City that Never Sleeps.mp3', 'FF9 - The Four Medallions.mp3', 'FF9 - Theme of Tantalus.mp3']
    music["yume"] = ['FF6 - Another World of Beasts.mp3', 'FF7 - Who Are You.mp3', 'FF8 - Drifting.mp3', 'FF8 - Lunatic Pandora.mp3', 'FF9 - A Transient Past.mp3', 'FF9 - Black Waltz.mp3', "FF9 - Cleyra's Trunk.mp3", 'FF9 - Footsteps of Desire.mp3', 'FF9 - Memories of That Day.mp3', 'FF9 - Queen of the Abyss.mp3', 'FF9 - The Chosen Summoner.mp3']

    for folder in music:
        target_path = "music/" + folder + "/"

        for file in music[folder]:
            src = "workspace/" + file[:3] + "/" + file
            if os.path.isdir("workspace/" + file[:3] + "_Normalized"):
                src = "workspace/" + file[:3] + "_Normalized/" + file
            dst = target_path + file
            shutil.copyfile(src, dst)

            # if we have a cfg file for this song we copy it.
            cfg_file = file.replace(".mp3", ".cfg")

            # if a song specific config exists use that, otherwise look for generic
            cfg_path = "configs/" + folder + "/" + cfg_file
            if not os.path.exists(cfg_path):
                cfg_path = "configs/" + cfg_file

            if os.path.exists(cfg_path):
                cfg_dst = target_path + cfg_file
                shutil.copyfile(cfg_path, cfg_dst)

print("Done.")