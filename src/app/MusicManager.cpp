#include "MusicManager.h"
#include "app/audio/AudioManager.h"
#include "app/gui/GUI.h"
#include "AppFrame/AppFrame.h"
#include "AppFrame/IconsFontAwesome5.h"
#include "LiveModFF7Core/game/GameManager.h"
#include "LiveModFF7Core/game/MemoryOffsets.h"
#include "LiveModFF7Core/utilities/Logging.h"
#include "mods/RandomizeMusic.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
namespace fs = std::filesystem;

static const char* Separator = "  \xC2\xB7  ";

static std::string toLower(std::string text)
{
    for (char& c : text) c = (char)std::tolower((unsigned char)c);
    return text;
}

static std::string formatTime(float seconds)
{
    int total = (int)std::max(seconds, 0.0f);
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%d:%02d", total / 60, total % 60);
    return buffer;
}

// e.g. "Fighting (bat)", or just the name for songs we don't have a title for.
static std::string describeSong(const std::string& name)
{
    std::string title = MusicLibrary::getSongTitle(name);
    return title == name ? name : title + " (" + name + ")";
}

// Found whether or not it's enabled, since it also owns the music volume setting.
static RandomizeMusic* getMusicMod()
{
    for (Mod* mod : Mod::getList())
    {
        if (mod->name == "Randomize Music")
        {
            return static_cast<RandomizeMusic*>(mod);
        }
    }

    return nullptr;
}

void MusicManager::draw(GameManager* game)
{
    if (!scanned)
    {
        scan();
    }

    // Follow whatever is playing, whether it was started here or by the randomizer.
    std::string path = AudioManager::getMusicPath();
    if (path != playingPath)
    {
        playingPath = path;
        playing = findTrack(path);
    }

    RandomizeMusic* musicMod = getMusicMod();

    ImGui::Spacing();
    drawNowPlaying(game, musicMod);
    ImGui::Spacing();
    drawTrackList(musicMod);
}

void MusicManager::drawNowPlaying(GameManager* game, RandomizeMusic* musicMod)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const LibraryTrack* track = getTrack(playing);
    const bool hasSong = !playingPath.empty();

    ImGui::BeginChild("##NowPlaying", ImVec2(0, 0), ImGuiChildFlags_Borders | ImGuiChildFlags_AutoResizeY);

    if (ImGui::BeginTable("##NowPlayingInfo", 2, ImGuiTableFlags_SizingFixedFit))
    {
        ImGui::TableSetupColumn("##Song", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("##Game", ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableNextRow();

        // Song title and the game song it replaces
        ImGui::TableNextColumn();
        ImGui::PushFont(nullptr, style.FontSizeBase * 1.4f);
        if (track != nullptr)
        {
            ImGui::TextUnformatted(track->title.c_str());
        }
        else if (hasSong)
        {
            ImGui::TextUnformatted(fs::path(playingPath).stem().string().c_str());
        }
        else
        {
            ImGui::TextDisabled("Nothing Playing");
        }
        ImGui::PopFont();

        if (track != nullptr)
        {
            std::string details = describeSong(library[playing.category].name);
            if (!track->source.empty())
            {
                details = track->source + Separator + details;
            }
            ImGui::TextDisabled("%s", details.c_str());
        }
        else if (hasSong)
        {
            // Not in the list, e.g. it was added to the folder since the last rescan.
            ImGui::TextDisabled("%s", playingPath.c_str());
        }
        else
        {
            ImGui::TextDisabled("Double-click a track below to play it.");
        }

        // The song the game itself is playing
        ImGui::TableNextColumn();
        if (game != nullptr)
        {
            uint16_t musicID = game->read<uint16_t>(GameOffsets::MusicID);
            ImGui::TextDisabled("In Game");
            if (musicID <= 1)
            {
                ImGui::TextUnformatted("No Music");
            }
            else if (musicID < MusicLibrary::MusicList.size())
            {
                ImGui::TextUnformatted(describeSong(MusicLibrary::MusicList[musicID]).c_str());
            }
            else
            {
                ImGui::Text("Unknown (%d)", musicID);
            }
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Seek bar: [position] [=====o-----] [length]
    {
        const float length = hasSong ? AudioManager::getMusicLength() : 0.0f;
        const float position = seeking ? seekPosition : (hasSong ? AudioManager::getMusicPosition() : 0.0f);
        const float timeWidth = ImGui::CalcTextSize("00:00").x;
        const float startX = ImGui::GetCursorPosX();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(formatTime(position).c_str());

        ImGui::SameLine(startX + timeWidth + style.ItemSpacing.x);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - timeWidth - style.ItemSpacing.x);
        ImGui::BeginDisabled(length <= 0.0f);
        {
            float value = std::min(position, length);
            ImGui::SliderFloat("##Seek", &value, 0.0f, std::max(length, 1.0f), "", ImGuiSliderFlags_NoInput);
            if (ImGui::IsItemActive())
            {
                seeking = true;
                seekPosition = value;
            }
            else if (seeking)
            {
                seeking = false;
                AudioManager::seekMusic(seekPosition);
            }
        }
        ImGui::EndDisabled();

        ImGui::SameLine();
        ImGui::TextUnformatted(length > 0.0f ? formatTime(length).c_str() : "-:--");
    }

    ImGui::Spacing();
    drawTransport(musicMod);

    ImGui::EndChild();
}

void MusicManager::drawTransport(RandomizeMusic* musicMod)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    const bool hasSong = !playingPath.empty();
    const bool hasTracks = trackCount > 0;
    const bool isPlaying = AudioManager::isMusicPlaying();

    const ImVec2 buttonSize(DPI(40.0f), 0.0f);
    const float transportWidth = (buttonSize.x * 4.0f) + (style.ItemSpacing.x * 3.0f);
    const float rowStartX = ImGui::GetCursorPosX();
    const float rowWidth = ImGui::GetContentRegionAvail().x;

    // Transport buttons in the middle: [prev] [play/pause] [next] [reroll]
    ImGui::SetCursorPosX(rowStartX + (rowWidth - transportWidth) * 0.5f);

    ImGui::BeginDisabled(!hasTracks);
    if (ImGui::Button(ICON_FA_STEP_BACKWARD "##Previous", buttonSize))
    {
        playAdjacent(-1);
    }
    ImGui::SetItemTooltip("Previous Track");
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!hasSong && !hasTracks);
    if (ImGui::Button(isPlaying ? ICON_FA_PAUSE "###PlayPause" : ICON_FA_PLAY "###PlayPause", buttonSize))
    {
        if (isPlaying)
        {
            AudioManager::pauseMusic();
        }
        else if (hasSong)
        {
            // Songs that play once stop at the end, so start them over.
            float length = AudioManager::getMusicLength();
            if (length > 0.0f && AudioManager::getMusicPosition() >= length - 0.1f)
            {
                AudioManager::seekMusic(0.0f);
            }
            AudioManager::resumeMusic();
        }
        else
        {
            play(selected.isValid() ? selected : TrackRef{ 0, 0 });
        }
    }
    ImGui::SetItemTooltip(isPlaying ? "Pause" : "Play");
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(!hasTracks);
    if (ImGui::Button(ICON_FA_STEP_FORWARD "##Next", buttonSize))
    {
        playAdjacent(1);
    }
    ImGui::SetItemTooltip("Next Track");
    ImGui::EndDisabled();

    ImGui::SameLine();
    ImGui::BeginDisabled(musicMod == nullptr || !musicMod->canReroll());
    if (ImGui::Button(ICON_FA_RANDOM "##Reroll", buttonSize))
    {
        musicMod->reroll();
    }
    ImGui::SetItemTooltip("Reroll\nPicks a new random track for the song playing in game.");
    ImGui::EndDisabled();

    // Volume on the right
    if (musicMod != nullptr)
    {
        const float sliderWidth = DPI(140.0f);
        const float iconWidth = ImGui::CalcTextSize(ICON_FA_VOLUME_UP).x;

        float percent = musicMod->getVolume() * 100.0f;
        const char* icon = ICON_FA_VOLUME_UP;
        if (percent <= 0.0f) icon = ICON_FA_VOLUME_MUTE;
        else if (percent < 50.0f) icon = ICON_FA_VOLUME_DOWN;

        ImGui::SameLine(rowStartX + rowWidth - sliderWidth - style.ItemSpacing.x - iconWidth);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(icon);

        ImGui::SameLine(rowStartX + rowWidth - sliderWidth);
        ImGui::SetNextItemWidth(sliderWidth);
        if (ImGui::SliderFloat("##Volume", &percent, 0.0f, RandomizeMusic::MaxVolume * 100.0f, "%.0f%%"))
        {
            musicMod->setVolume(percent / 100.0f);
        }
    }
}

void MusicManager::drawTrackList(RandomizeMusic* musicMod)
{
    const ImGuiStyle& style = ImGui::GetStyle();

    // Toolbar: [search] [count] ... [rescan]
    {
        const float rowStartX = ImGui::GetCursorPosX();
        const float rowWidth = ImGui::GetContentRegionAvail().x;

        ImGui::SetNextItemWidth(DPI(250.0f));
        ImGui::InputTextWithHint("##Search", ICON_FA_SEARCH "  Search", filter, sizeof(filter));
        filterLower = toLower(filter);

        ImGui::SameLine();
        ImGui::AlignTextToFramePadding();
        ImGui::TextDisabled("%d tracks for %d songs", trackCount, (int)library.size());

        const char* rescanLabel = ICON_FA_SYNC "  Rescan";
        const float rescanWidth = ImGui::CalcTextSize(rescanLabel).x + (style.FramePadding.x * 2.0f);
        ImGui::SameLine(rowStartX + rowWidth - rescanWidth);
        if (ImGui::Button(rescanLabel))
        {
            scan();
            if (musicMod != nullptr)
            {
                musicMod->rescanMusicFolder();
            }
        }
        ImGui::SetItemTooltip("Reloads the music folder for both this list and the randomizer.");
    }

    if (!folderFound)
    {
        ImGui::TextWrapped("The music folder wasn't found. See tools/music for the script that builds it.");
        return;
    }

    if (trackCount == 0)
    {
        ImGui::TextWrapped("The music folder doesn't have any tracks.");
        return;
    }

    const ImGuiTableFlags tableFlags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV;
    if (!ImGui::BeginTable("##Tracks", 3, tableFlags, ImVec2(0.0f, ImGui::GetContentRegionAvail().y)))
    {
        return;
    }

    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Game", ImGuiTableColumnFlags_WidthFixed, DPI(60.0f));
    ImGui::TableSetupColumn("Loop", ImGuiTableColumnFlags_WidthFixed, DPI(60.0f));
    ImGui::TableHeadersRow();

    const ImVec4 accent = ImGui::GetStyleColorVec4(ImGuiCol_CheckMark);
    const bool filtering = !filterLower.empty();

    // Game songs, with the tracks that replace them underneath.
    for (int c = 0; c < (int)library.size(); ++c)
    {
        const LibraryCategory& category = library[c];

        // When searching, a matching song shows all of its tracks, otherwise only the matching tracks are shown.
        const bool showAllTracks = categoryMatches(c);
        if (!showAllTracks)
        {
            bool anyMatches = false;
            for (int t = 0; t < (int)category.tracks.size() && !anyMatches; ++t)
            {
                anyMatches = trackMatches({ c, t });
            }

            if (!anyMatches)
            {
                continue;
            }
        }

        ImGui::PushID(c);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Results are always expanded. They use their own IDs so the list goes back to how it was once the search is cleared.
        if (filtering)
        {
            ImGui::SetNextItemOpen(true);
        }

        const bool hasPlaying = playing.category == c;
        if (hasPlaying) ImGui::PushStyleColor(ImGuiCol_Text, accent);
        const bool open = ImGui::TreeNodeEx(filtering ? "##Result" : "##Song", ImGuiTreeNodeFlags_SpanAllColumns, "%s%s",
            hasPlaying ? ICON_FA_VOLUME_UP "  " : "", category.title.c_str());
        if (hasPlaying) ImGui::PopStyleColor();

        std::string details = std::to_string(category.tracks.size());
        if (category.title != category.name)
        {
            details = category.name + Separator + details;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("%s", details.c_str());

        if (open)
        {
            for (int t = 0; t < (int)category.tracks.size(); ++t)
            {
                const TrackRef ref = { c, t };
                if (!showAllTracks && !trackMatches(ref))
                {
                    continue;
                }

                const LibraryTrack& entry = category.tracks[t];
                const bool isPlaying = playing == ref;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAllColumns;
                if (selected == ref)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::PushID(t);
                ImGui::TableNextRow();
                ImGui::TableNextColumn();

                if (isPlaying) ImGui::PushStyleColor(ImGuiCol_Text, accent);
                ImGui::TreeNodeEx("##Track", flags, "%s%s", isPlaying ? ICON_FA_VOLUME_UP "  " : "", entry.title.c_str());
                if (isPlaying) ImGui::PopStyleColor();

                if (ImGui::IsItemClicked())
                {
                    selected = ref;
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                {
                    play(ref);
                }

                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry.source.c_str());

                ImGui::TableNextColumn();
                if (entry.track.playOnce)
                {
                    ImGui::TextDisabled("Once");
                    ImGui::SetItemTooltip("Plays once without looping.");
                }
                else if (entry.track.loopStart > 0 || entry.track.loopEnd != UINT64_MAX)
                {
                    ImGui::TextDisabled("Custom");
                    ImGui::SetItemTooltip("Loops between the points set in its .cfg file.");
                }
                else
                {
                    ImGui::TextDisabled("Full");
                    ImGui::SetItemTooltip("Loops the whole track.");
                }

                ImGui::PopID();
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::EndTable();
}

void MusicManager::scan()
{
    scanned = true;
    library.clear();
    trackCount = 0;
    selected = {};
    playing = {};
    playingPath.clear();

    TrackMap tracks;
    folderFound = MusicLibrary::scan(tracks);

    for (const auto& [name, songTracks] : tracks)
    {
        LibraryCategory category;
        category.name = name;
        category.title = MusicLibrary::getSongTitle(name);
        category.searchText = toLower(category.title + "\n" + name);

        for (const Track& track : songTracks)
        {
            LibraryTrack entry;
            entry.track = track;

            // Files are named "<Game> - <Title>", e.g. "FF6 - Battle Theme".
            const std::string fileName = fs::path(track.path).stem().string();
            const size_t split = fileName.find(" - ");
            if (split != std::string::npos)
            {
                entry.source = fileName.substr(0, split);
                entry.title = fileName.substr(split + 3);
            }
            else
            {
                entry.title = fileName;
            }
            entry.searchText = toLower(fileName);

            category.tracks.push_back(entry);
        }

        trackCount += (int)category.tracks.size();
        library.push_back(std::move(category));
    }

    // Folder names are cryptic, so list the songs by title.
    std::sort(library.begin(), library.end(), [](const LibraryCategory& a, const LibraryCategory& b) { return a.searchText < b.searchText; });
}

void MusicManager::play(TrackRef ref)
{
    const LibraryTrack* entry = getTrack(ref);
    if (entry == nullptr)
    {
        return;
    }

    selected = ref;

    const Track& track = entry->track;
    if (!AudioManager::playMusic(track.path, track.start, track.loopStart, track.loopEnd, track.playOnce, track.noFade))
    {
        LOG("Music Manager: failed to play %s", track.path.c_str());
    }
}

void MusicManager::playAdjacent(int direction)
{
    if (trackCount == 0)
    {
        return;
    }

    // Step through the list in order, wrapping around at either end and skipping anything hidden by the search.
    const int categoryCount = (int)library.size();
    TrackRef ref = playing.isValid() ? playing : selected;
    if (!ref.isValid())
    {
        ref = { 0, direction > 0 ? -1 : 0 };
    }

    for (int i = 0; i < trackCount; ++i)
    {
        ref.track += direction;
        if (ref.track < 0)
        {
            ref.category = (ref.category + categoryCount - 1) % categoryCount;
            ref.track = (int)library[ref.category].tracks.size() - 1;
        }
        else if (ref.track >= (int)library[ref.category].tracks.size())
        {
            ref.category = (ref.category + 1) % categoryCount;
            ref.track = 0;
        }

        if (categoryMatches(ref.category) || trackMatches(ref))
        {
            play(ref);
            return;
        }
    }
}

const MusicManager::LibraryTrack* MusicManager::getTrack(TrackRef ref) const
{
    if (ref.category < 0 || ref.category >= (int)library.size())
    {
        return nullptr;
    }

    const std::vector<LibraryTrack>& tracks = library[ref.category].tracks;
    if (ref.track < 0 || ref.track >= (int)tracks.size())
    {
        return nullptr;
    }

    return &tracks[ref.track];
}

MusicManager::TrackRef MusicManager::findTrack(const std::string& path) const
{
    if (path.empty())
    {
        return {};
    }

    for (int c = 0; c < (int)library.size(); ++c)
    {
        for (int t = 0; t < (int)library[c].tracks.size(); ++t)
        {
            if (library[c].tracks[t].track.path == path)
            {
                return { c, t };
            }
        }
    }

    return {};
}

bool MusicManager::categoryMatches(int category) const
{
    return filterLower.empty() || library[category].searchText.find(filterLower) != std::string::npos;
}

bool MusicManager::trackMatches(TrackRef ref) const
{
    const LibraryTrack* entry = getTrack(ref);
    return entry != nullptr && entry->searchText.find(filterLower) != std::string::npos;
}
