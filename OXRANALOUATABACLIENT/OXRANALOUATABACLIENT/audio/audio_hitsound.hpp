#ifndef AUDIO_HITSOUND_HPP
#define AUDIO_HITSOUND_HPP

// Hit sound audio: DirectSound backend с контролем громкости.
//
// Сделано через DirectSound (dsound.dll, есть на всех Windows). Линкуется
// dsound.lib в core_main.cpp. Дёргаем сырое memory (WAV resource уже в памяти)
// и грузим в секундарный DSound buffer с возможностью volume control в
// диапазоне [0..100]% через IDirectSoundBuffer::SetVolume (дБ).

namespace audio {

// Вызвать один раз при старте после того как g_hitSoundBuffer и g_hitSoundSize
// заполнены (из WAV resource). Возвращает true если backend готов.
bool InitHitSound(const void* wavData, unsigned long wavSize);

// Проиграть hit sound. Громкость 0..100 (%). Неблокирующе.
void PlayHitSound(int volumePercent);

// Освободить ресурсы.
void ShutdownHitSound();

} // namespace audio

#endif // AUDIO_HITSOUND_HPP
