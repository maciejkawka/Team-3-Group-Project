#pragma once
#include"SoundManager.h"

namespace NCL {
	namespace CSC8508 {
		namespace Audio {
			
			struct FMOD_3DAttributes {

				FMOD_VECTOR pos;
				FMOD_VECTOR vel;
			};
			
			class SoundInstance {
			public:

				SoundInstance() {}
				~SoundInstance() {} //release

				int Play();
				int Stop();
				int isPlaying();
				
				void SetPaused();
				bool isPaused();

				void SetVolume();
				float GetVolume();
				void SetPitch();
				float GetPith();

				void Set3DAtribbutes();

				static FMOD_VECTOR ToFMODVECTOR(const Maths::Vector3& v);
				static float FloatTodB(float volume);
				static float dBtoFloat(float dB);

			protected:

				FMOD::Sound* sound;
				FMOD::System* system;
				FMOD_VECTOR position;
				FMOD_3DAttributes attributes3D;
				float volume;
				float pitch;

				friend void SoundManager::CreateInstance(const std::string& path, SoundInstance* soundInstnce);
			
			};
		}
	}
}
