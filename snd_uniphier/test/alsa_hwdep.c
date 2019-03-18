#include <stdlib.h>
#include <stdint.h>

#include <sys/ioctl.h>

#include <alsa/asoundlib.h>

#define SNDRV_HWDEP_IOCTL_MN2WS_GET_MUTE_PCM    _IOR('H', 0x80, int *)
#define SNDRV_HWDEP_IOCTL_MN2WS_SET_MUTE_PCM    _IOW('H', 0x81, int *)
#define SNDRV_HWDEP_IOCTL_MN2WS_GET_MUTE_DEC    _IOR('H', 0x82, int *)
#define SNDRV_HWDEP_IOCTL_MN2WS_SET_MUTE_DEC    _IOW('H', 0x83, int *)

#define SNDRV_HWDEP_IOCTL_MN2WS_FORCE_AOUT      _IO('H', 0xf0)

int change_audio_mute(snd_hwdep_t *hwdep, int *mute_pcm, int *mute_dec);
int force_audio_out(snd_hwdep_t *hwdep);

int main(int argc, char *argv[])
{
	const char *device = "default";
	int command, mute_pcm, mute_dec, force_aout;
	int mute_pcm_before, mute_dec_before;
	int mute_pcm_after, mute_dec_after;
	snd_hwdep_t *hwdep = NULL;
	int result = -1;

	if (argc < 2) {
		fprintf(stderr, "usage: \n%s command [device]\n"
			"  command:  0: PCM on,  DEC on\n"
			"            1: PCM off, DEC on\n"
			"            2: PCM on,  DEC off\n"
			"            3: PCM off, DEC off\n"
			"           99: force audio out setting(for debug)\n"
			"  device : ALSA device(ex. hw:0, hw:1, ...)\n", 
			argv[0]);
		return 1;
	}

	if (argc >= 2) {
		command = strtol(argv[1], NULL, 0);
	}
	if (argc >= 3) {
		device = argv[2];
	}

	//convert arguments
	mute_pcm = 0;
	mute_dec = 0;
	force_aout = 0;
	switch (command) {
	case 0:
		break;
	case 1:
		mute_pcm = 1;
		break;
	case 2:
		mute_dec = 1;
		break;
	case 3:
		mute_pcm = 1;
		mute_dec = 1;
		break;
	case 99:
		force_aout = 1;
		break;
	default:
		fprintf(stderr, "invalid command %d.\n", command);
		return 1;
	}

	//open the hwdep ALSA device
	result = snd_hwdep_open(&hwdep, device, 0);
	if (result < 0) {
		perror("snd_hwdep_open()");
		goto err_out;
	}

	//get before
	result = snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_MN2WS_GET_MUTE_PCM, 
		(void *)&mute_pcm_before);
	if (result < 0) {
		perror("snd_hwdep_ioctl(GET_MUTE_PCM, before)");
		goto err_out;
	}

	result = snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_MN2WS_GET_MUTE_DEC, 
		(void *)&mute_dec_before);
	if (result < 0) {
		perror("snd_hwdep_ioctl(GET_MUTE_DEC, before)");
		goto err_out;
	}

	//exec command
	switch (command) {
	case 0:
	case 1:
	case 2:
	case 3:
		printf("command: %d: change mute\n", command);
		result = change_audio_mute(hwdep, &mute_pcm, &mute_dec);
		break;
	case 99:
		printf("command: %d: force aout\n", command);
		result = force_audio_out(hwdep);
		break;
	}
	if (result < 0) {
		goto err_out;
	}

	//get after
	result = snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_MN2WS_GET_MUTE_PCM, 
		(void *)&mute_pcm_after);
	if (result < 0) {
		perror("snd_hwdep_ioctl(GET_MUTE_PCM, after)");
		goto err_out;
	}

	result = snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_MN2WS_GET_MUTE_DEC, 
		(void *)&mute_dec_after);
	if (result < 0) {
		perror("snd_hwdep_ioctl(GET_MUTE_DEC, after)");
		goto err_out;
	}

	printf("device : '%s'\n"
		"  pcm  : %s -> %s\n"
		"  dec  : %s -> %s\n", 
		device, 
		(!mute_pcm_before) ? "ON " : "OFF", 
		(!mute_pcm_after)  ? "ON " : "OFF", 
		(!mute_dec_before) ? "ON " : "OFF", 
		(!mute_dec_after)  ? "ON " : "OFF");

	result = 0;

err_out:
	if (hwdep) {
		snd_hwdep_close(hwdep);
		hwdep = NULL;
	}

	return result;
}

int change_audio_mute(snd_hwdep_t *hwdep, int *mute_pcm, int *mute_dec)
{
	int result;

	result = snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_MN2WS_SET_MUTE_PCM, 
		(void *)mute_pcm);
	if (result < 0) {
		perror("snd_hwdep_ioctl(SET_MUTE_PCM)");
		goto err_out;
	}

	result = snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_MN2WS_SET_MUTE_DEC, 
		(void *)mute_dec);
	if (result < 0) {
		perror("snd_hwdep_ioctl(SET_MUTE_DEC)");
		goto err_out;
	}

	return 0;

err_out:
	return result;
}

int force_audio_out(snd_hwdep_t *hwdep)
{
	int result;

	result = snd_hwdep_ioctl(hwdep, SNDRV_HWDEP_IOCTL_MN2WS_FORCE_AOUT, 
		NULL);
	if (result < 0) {
		perror("snd_hwdep_ioctl(FORCE_AOUT)");
		goto err_out;
	}

	return 0;

err_out:
	return result;
}
