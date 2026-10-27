#include "MusicLibrary.h"
#include "LiveModFF7Core/utilities/Utilities.h"
#include "utilities/ConfigFile.h"

#include <cctype>
#include <filesystem>
#include <unordered_map>
namespace fs = std::filesystem;

const std::string MusicLibrary::MusicFolder = "music";

const std::vector<std::string> MusicLibrary::MusicList = {
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
};

// From the table in music/README.md.
static const std::unordered_map<std::string, std::string> SongTitles = {
    { "aseri", "Hurry!" },
    { "aseri2", "Hurry, Faster!" },
    { "ayasi", "Lurking in the Darkness" },
    { "barret", "Barret's Theme" },
    { "bat", "Fighting" },
    { "bee", "Honeybee Manor" },
    { "bokujo", "Farm Boy" },
    { "boo", "Life Stream" },
    { "cannon", "The Mako Cannon Fires!" },
    { "canyon", "Cosmo Canyon" },
    { "cephiros", "Those Chosen By the Planet" },
    { "chase", "Crazy Motorcycle Chase" },
    { "chu", "Still More Fighting" },
    { "chu2", "J-E-N-O-V-A" },
    { "cinco", "Cinco de Chocobo" },
    { "cintro", "Those Chosen By the Planet" },
    { "comical", "Comical (deleted song)" },
    { "condor", "Fortress of the Condor" },
    { "corel", "Mining Town" },
    { "corneo", "Don of the Slums" },
    { "costa", "Costa del Sol" },
    { "crlost", "Tango of Tears" },
    { "crwin", "A Great Success" },
    { "date", "Interrupted by Fireworks" },
    { "dokubo", "Underneath the Rotting Pizza" },
    { "dun2", "Chasing the Black-Caped Man" },
    { "earis", "Aeris' Theme" },
    { "earislo", "Flowers Blooming in the Church" },
    { "elec", "Electric de Chocobo" },
    { "fan2", "Fanfare" },
    { "fanfare", "Fanfare" },
    { "fiddle", "Fiddle de Chocobo" },
    { "fin", "World Crisis" },
    { "geki", "Debut" },
    { "gold1", "Gold Saucer" },
    { "guitar2", "On the Other Side of the Mountain" },
    { "gun", "Full-Scale Attack" },
    { "hen", "Who Am I" },
    { "hiku", "The Highwind Takes to the Skies" },
    { "horror", "Trail of Blood" },
    { "iseki", "You Can Hear the Cry of the Planet" },
    { "jukai", "Forested Temple" },
    { "junon", "Off the Edge of Despair" },
    { "jyro", "Steal the Tiny Bronco!" },
    { "ketc", "Cait Sith's Theme" },
    { "kita", "The Great Northern Cave" },
    { "kurai", "Anxious Heart" },
    { "lb1", "The Birth of God" },
    { "lb2", "One-Winged Angel" },
    { "ld", "Judgement Day" },
    { "makoro", "Mako Reactor" },
    { "mati", "Ahead on Our Way" },
    { "mekyu", "Reunion" },
    { "mogu", "The Highwind Takes to the Skies (Moggle version)" },
    { "mura1", "Parochial Town" },
    { "nointro", "Shinra Corporation" },
    { "oa", "Opening ~ Bombing Mission" },
    { "ob", "Bombing Mission" },
    { "odds", "Cinco de Chocobo" },
    { "over2", "Continue" },
    { "parade", "Rufus' Welcoming Ceremony" },
    { "pj", "Jenova Absolute" },
    { "pre", "The Prelude" },
    { "red", "Red XIII's Theme" },
    { "rhythm", "Turks' Theme" },
    { "riku", "Reunion" },
    { "ro", "The Countdown Begins" },
    { "rocket", "Oppressed People" },
    { "roll", "Staff Roll" },
    { "rukei", "Sandy Badlands" },
    { "sadbar", "Mark of the Traitor" },
    { "sadsid", "Sending a Dream into the Universe" },
    { "sea", "A Secret Sleeping in the Deep Sea" },
    { "seto", "Great Warrior" },
    { "si", "Reunion" },
    { "sid2", "Cid's Theme" },
    { "sido", "It's Difficult to Stand on Both Feet, Isn't It" },
    { "siera", "If You Open Your Heart..." },
    { "sinra", "Shinra Corporation" },
    { "sinraslo", "Infiltrating Shinra Tower" },
    { "snow", "Buried in the Snow" },
    { "ta", "Main Theme of Final Fantasy VII" },
    { "tb", "Main Theme of Final Fantasy VII (alternate)" },
    { "tender", "Holding My Thoughts in My Heart" },
    { "tifa", "Tifa's Theme" },
    { "tm", "On That Day, 5 Years Ago" },
    { "utai", "Wutai" },
    { "vincent", "The Nightmare's Beginning" },
    { "walz", "Waltz de Chocobo" },
    { "weapon", "Weapon Raid" },
    { "wind", "Wind" },
    { "yado", "Good Night, Until Tomorrow!" },
    { "yufi", "Descendant of Shinobi" },
    { "yufi2", "Stolen Materia" },
    { "yume", "Who Are You" },
    { "snowboarding", "Snowboarding" },
};

std::string MusicLibrary::getSongTitle(const std::string& name)
{
    auto it = SongTitles.find(name);
    return it != SongTitles.end() ? it->second : name;
}

bool MusicLibrary::scan(TrackMap& tracks)
{
    tracks.clear();

    if (!fs::exists(MusicFolder) || !fs::is_directory(MusicFolder))
    {
        return false;
    }

    for (const auto& musicEntry : fs::directory_iterator(MusicFolder))
    {
        if (!musicEntry.is_directory())
        {
            continue;
        }

        const std::string name = musicEntry.path().filename().string();

        // This is to prevent someone from overriding silence.
        if (name == "none" || name == "nothing")
        {
            continue;
        }

        for (const auto& entry : fs::directory_iterator(musicEntry.path()))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::string ext = entry.path().extension().string();
            for (char& c : ext) c = std::tolower(c);

            if (ext == ".mp3" || ext == ".wav")
            {
                tracks[name].push_back(loadTrack(entry.path().string()));
            }
        }
    }

    return true;
}

Track MusicLibrary::loadTrack(const std::string& path)
{
    Track track;
    track.path = path;

    std::string cfgFilename = Utilities::replaceExtension(path, ".mp3", ".cfg");
    cfgFilename = Utilities::replaceExtension(cfgFilename, ".wav", ".cfg");

    ConfigFile cfg;
    if (!cfg.load(cfgFilename))
    {
        return track;
    }

    track.start     = cfg.get<uint64_t>("Start", 0);
    track.loopStart = cfg.get<uint64_t>("LoopStart", 0);
    track.loopEnd   = cfg.get<uint64_t>("LoopEnd", UINT64_MAX);
    track.playOnce  = cfg.get<bool>("PlayOnce", false);
    track.noFade    = cfg.get<bool>("NoFade", false);

    return track;
}
