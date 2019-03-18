#include <stdlib.h>
#include <stdint.h>

#include <alsa/asoundlib.h>

#define ENUM_NAMELEN    256

int main(int argc, char *argv[])
{
	const char *device_name = "default";
	const char *command;
	snd_mixer_t *mix = NULL;
	struct snd_mixer_selem_regopt *mix_opt = NULL;
	snd_mixer_class_t *mix_classp = NULL;
	snd_mixer_selem_id_t *mix_sid;
	const char *mix_selem_name = "Master";
	snd_mixer_elem_t *mix_elem = NULL;
	long vol_rate, mix_vol_min, mix_vol_max, mix_vol;
	char enum_name[ENUM_NAMELEN];
	int cnt, i, result = -1;

	if (argc < 2) {
		fprintf(stderr, "usage: \n%s command [device] [mixer]\n"
			"  command: 0-100: volume\n"
			"  device : ALSA device(ex. hw:0, hw:1, ...)\n"
			"  mixer  : Mixer element(ex. Master, ...)\n",
			argv[0]);
		return 1;
	}

	if (argc >= 2) {
		command = argv[1];
	}
	if (argc >= 3) {
		device_name = argv[2];
	}
	if (argc >= 4) {
		mix_selem_name = argv[3];
	}

	printf("command: %s, device: %s, mixer: %s.\n",
		command, device_name, mix_selem_name);

	//open the mixer
	result = snd_mixer_open(&mix, 0);
	if (result < 0) {
		perror("snd_mixer_open()");
		goto err_out;
	}

	result = snd_mixer_attach(mix, device_name);
	if (result < 0) {
		perror("snd_mixer_attach()");
		goto err_out;
	}

	result = snd_mixer_selem_register(mix, mix_opt, &mix_classp);
	if (result < 0) {
		perror("snd_mixer_selem_register()");
		goto err_out;
	}

	result = snd_mixer_load(mix);
	if (result < 0) {
		perror("snd_mixer_load()");
		goto err_out;
	}

	//set selem_id
	snd_mixer_selem_id_alloca(&mix_sid);
	snd_mixer_selem_id_set_index(mix_sid, 0);
	snd_mixer_selem_id_set_name(mix_sid, mix_selem_name);

	//find mixer element
	mix_elem = snd_mixer_find_selem(mix, mix_sid);
	if (mix_elem == NULL) {
		perror("snd_mixer_find_selem()");
		goto err_out;
	}

	result = 0;

	if (snd_mixer_selem_has_playback_volume(mix_elem)) {
		printf("type: playback_volume.\n");

		vol_rate = strtol(command, NULL, 0);

		result = snd_mixer_selem_get_playback_volume_range(mix_elem,
			&mix_vol_min, &mix_vol_max);
		if (result < 0) {
			perror("snd_mixer_selem_get_playback_volume_range()");
			goto err_out;
		}

		mix_vol = mix_vol_min + (mix_vol_max - mix_vol_min) * vol_rate / 100;
		printf("min: %ld, max: %ld, vol: %ld.\n",
			mix_vol_min, mix_vol_max, mix_vol);

		result = snd_mixer_selem_set_playback_volume_all(mix_elem, mix_vol);
		if (result < 0) {
			perror("snd_mixer_selem_set_playback_volume_all()");
			goto err_out;
		}
	} else if (snd_mixer_selem_has_capture_volume(mix_elem)) {
		printf("type: capture_volume.\n");

		vol_rate = strtol(command, NULL, 0);

		result = snd_mixer_selem_get_capture_volume_range(mix_elem,
			&mix_vol_min, &mix_vol_max);
		if (result < 0) {
			perror("snd_mixer_selem_get_capture_volume_range()");
			goto err_out;
		}

		mix_vol = mix_vol_min + (mix_vol_max - mix_vol_min) * vol_rate / 100;
		printf("min: %ld, max: %ld, vol: %ld.\n",
			mix_vol_min, mix_vol_max, mix_vol);

		result = snd_mixer_selem_set_capture_volume_all(mix_elem, mix_vol);
		if (result < 0) {
			perror("snd_mixer_selem_set_capture_volume_all()");
			goto err_out;
		}
	} else if (snd_mixer_selem_is_enumerated(mix_elem)) {
		printf("type: enum.\n");

		cnt = snd_mixer_selem_get_enum_items(mix_elem);
		if (cnt < 0) {
			perror("snd_mixer_selem_get_enum_items()");
			result = -1;
			goto err_out;
		}

		for (i = 0; i < cnt; i++) {
			result = snd_mixer_selem_get_enum_item_name(mix_elem, i,
				ENUM_NAMELEN, enum_name);
			if (result < 0) {
				perror("snd_mixer_selem_get_enum_item_name()");
				goto err_out;
			}
			printf("  %s\n", enum_name);
		}

		for (i = 0; i < cnt; i++) {
			snd_mixer_selem_get_enum_item_name(mix_elem, i,
				ENUM_NAMELEN, enum_name);
			if (strncasecmp(command, enum_name, ENUM_NAMELEN) == 0) {
				printf("found. name:%s, index:%d\n", enum_name, i);
				break;
			}
		}
		if (i == cnt) {
			printf("not found %s\n", command);
			result = -1;
			goto err_out;
		}

		result = snd_mixer_selem_set_enum_item(mix_elem, 0, i);
		if (result < 0) {
			perror("snd_mixer_selem_set_enum_items()");
			goto err_out;
		}
	} else {
		printf("type: unknown.\n");
		result = -1;
	}

err_out:
	if (mix) {
		snd_mixer_close(mix);
		mix = NULL;
	}

	return result;
}
