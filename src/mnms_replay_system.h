// src/mnms_replay_system.h
#ifndef MNMS_REPLAY_SYSTEM_H
#define MNMS_REPLAY_SYSTEM_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

class MnmsReplaySystem : public Node {
    GDCLASS(MnmsReplaySystem, Node)

private:
    bool recording;
    Array recorded_frames;

protected:
    static void _bind_methods();

public:
    MnmsReplaySystem();
    ~MnmsReplaySystem();

    void start_recording();
    void stop_recording();
    void set_recording(bool p_recording);
    void clear();
    void push_frame(const Dictionary &p_frame);
    void set_recorded_frames(const Array &p_frames);
    Array get_recorded_frames() const;
    Dictionary load_from_file(const String &p_path);
    bool save_to_file(const String &p_path, const Dictionary &p_metadata = Dictionary()) const;
    bool is_recording() const;
    int32_t get_frame_count() const;
};

} // namespace godot

#endif
