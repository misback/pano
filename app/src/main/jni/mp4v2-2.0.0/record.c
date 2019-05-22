#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <sys/dir.h>
#include <sys/statfs.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include "../../interface/inc/msg_def.h"
#include "../../interface/inc/interface.h"
#include "message.h"
#include "record.h"
#include "mp4v2/mp4v2.h"

/* #define RECORD_DEBUG */

static int h264_field[7][3] =
{
	{AV_OP_LOCK_H264_VOL,			AV_OP_LOCK_H264_CIF_VOL,			-1},
	{AV_OP_UNLOCK_H264_VOL,			AV_OP_UNLOCK_H264_CIF_VOL,			-1},
	{AV_OP_LOCK_H264,				AV_OP_LOCK_H264_CIF,				AV_OP_LOCK_MJPEG},
	{AV_OP_LOCK_H264_IFRAME, 		AV_OP_LOCK_H264_CIF_IFRAME,			AV_OP_LOCK_MJPEG},
	{AV_OP_UNLOCK_H264,				AV_OP_UNLOCK_H264_CIF,				AV_OP_UNLOCK_MJPEG},
	{AV_OP_GET_H264_SERIAL,			AV_OP_GET_H264_CIF_SERIAL,			AV_OP_GET_MJPEG_SERIAL},
	{AV_OP_WAIT_NEW_H264_SERIAL,	AV_OP_WAIT_NEW_H264_CIF_SERIAL,		AV_OP_WAIT_NEW_MJPEG_SERIAL}
};

#define LOCK_H264_VOL			0
#define UNLOCK_H264_VOL			1
#define LOCK_H264				2
#define LOCK_H264_IFRAM			3
#define UNLOCK_H264				4
#define GET_H264_SERIAL			5
#define WAIT_NEW_H264_SERIAL	6

#define PRE_BUFFER	25

#define MMC_PATH	"/mnt/mmc"
#define TMP_PATH	"/tmp"
const char *curr_path = NULL;
int abort_signal = 0;
int video_serial_start = -1;
int audio_serial_start = -1;

int sps_size = 52;
int pps_size = 4;

static void signal_handler(int signum)
{
	dbg_printf("record caught SIGTERM: shutting down\n");
	abort_signal = 1;
}

void init_signals(void)
{
    signal(SIGINT, signal_handler);

	abort_signal = 0;
}

long long get_disk_freespace(const char *disk)
{
	long long int freespace = 0;
	struct statfs disk_statfs;

	if( statfs(disk, &disk_statfs) >= 0 )
	{
		freespace = (((long long int)disk_statfs.f_bsize  * (long long int)disk_statfs.f_bfree)/(long long int)1024);
	}

	return freespace;
}

int is_limit_reach = 0;
int is_full = 0;
int reduce_limit_space_flag = 0;

int check_reserve_space(int is_card, long long int reserve_space, long long int *limit_space)
{
	long long int minspace = 0;
	long long int maxspace = 0;
	long long int freespace = get_disk_freespace(curr_path);
	dbg_printf("Space on %s = %lld Kbyte \n", curr_path, freespace);

	if(is_card)
	{
		minspace = reserve_space / 1024;
		maxspace = (*limit_space + reserve_space) / 1024;
	}
	else
	{
		minspace = 4 * 1024;
	}

	dbg_printf("minspace %lld\n", minspace);

	if(freespace < minspace)
	{
		dbg_printf("Space = %lld Kbyte is not enough, need %lld \n", freespace, minspace);

        is_full = 1;

		return -1;
	}
	else if(freespace < maxspace)
	{
		dbg_printf("freespace = %lld Kbyte is not enough, need %lld, reduce limit_space to %lld\n", 
				   freespace, maxspace, (freespace - minspace));

		*limit_space = (freespace - minspace) * 1024;
		reduce_limit_space_flag = 1;
	}

	return 0;
}

int get_video_info(int stream_id, AV_DATA *info )
{
	if(GetAVData(h264_field[LOCK_H264_VOL][stream_id], -1, info ) < 0)
	{
		return -1;
	}

	return 0;
}

void wait_video_start(int stream_id, int frame_rate)
{
	int ret = 0;
	int cnt = 0;
	int got_serial = 0;
	unsigned int serial_lock = 0;
	AV_DATA av_data;

	GetAVData(h264_field[GET_H264_SERIAL][stream_id], -1, &av_data);
	serial_lock = av_data.serial - frame_rate;

	while(abort_signal == 0)
	{
		ret = GetAVData(h264_field[LOCK_H264][stream_id], serial_lock, &av_data);
		if(ret == RET_SUCCESS)
		{
			GetAVData(h264_field[UNLOCK_H264][stream_id], serial_lock, &av_data);
			serial_lock++;

			if(av_data.flags != AV_FLAGS_H264_I_FRAME)
			{
				continue;
			}
			else
			{
				video_serial_start = av_data.ref_serial[FMT_H264];
				audio_serial_start = av_data.ref_serial[FMT_AUDIO];

				cnt++;
				got_serial = 1;

				dbg_printf("video_serial_start %d audio_serial_start %d\n", video_serial_start, audio_serial_start);

				break;
			}
		}
		else if(ret == RET_NO_VALID_DATA)
		{
			if(got_serial)
			{
				break;
			}

			usleep(40 * 1000);
		}
		else
		{
			GetAVData(h264_field[GET_H264_SERIAL][stream_id], -1, &av_data );
			serial_lock = av_data.serial - frame_rate;

			usleep(40 * 1000);

			dbg_printf("av_data.serial %d serial_lock %d\n", av_data.serial, serial_lock);
		}
	}
}

int get_audio(unsigned char *audio_buf, unsigned int *audio_size, unsigned int *audio_time)
{
	int error_code = 0;
	static int	serial_lock = -1;
	int	ret = 0;
	AV_DATA av_data;

	while(1)
	{
		if(serial_lock < 0)
		{
			serial_lock = audio_serial_start;
		}

		ret = GetAVData(AV_OP_LOCK_ULAW, serial_lock, &av_data);

		if(ret == RET_SUCCESS)
		{
			memcpy(audio_buf, av_data.ptr, av_data.size);

			GetAVData(AV_OP_UNLOCK_ULAW, serial_lock, &av_data);

			serial_lock++;

			*audio_size = (int)av_data.size;
			*audio_time = av_data.timestamp;

			return 0;
		}
		else if (ret == RET_NO_VALID_DATA)
		{
			return -1;
		}
		else 
		{
			GetAVData(AV_OP_GET_ULAW_SERIAL, -1, &av_data);
			serial_lock = av_data.serial;
			return -1;
		}
	}

	return error_code;
}

static int write_audio_frame(MP4FileHandle mp4_file_handle, MP4TrackId audio_track, unsigned char *audio_buf, unsigned int *audio_size, unsigned int *audio_time)
{
	int ret = -1;
	unsigned long long int duration = MP4_INVALID_DURATION;
	static int prev_audio_time = 0;

	if(get_audio(audio_buf, audio_size, audio_time) < 0)
	{
#ifdef RECORD_DEBUG
		dbg_printf("audio no data!\n");
#endif
		return -1;
	}

#ifdef RECORD_DEBUG
	struct timeval audio_start, audio_end;
	gettimeofday(&audio_start, NULL);
#endif

	if(prev_audio_time == 0)
	{
		prev_audio_time = *audio_time;
	}
	else
	{
		duration = (*audio_time - prev_audio_time) * 1000 / 5625;
		prev_audio_time = *audio_time;
	}
	/* dbg_printf("audio duration %d\n", *audio_time - prev_audio_time); */

	ret = MP4WriteSample(mp4_file_handle, audio_track, audio_buf, *audio_size, duration, 0, 1);
	if(ret < 0)
	{
        dbg_printf( "write_audio failed, ret %d\n", ret);
        exit(1);
    }

#ifdef RECORD_DEBUG
	gettimeofday(&audio_end, NULL);
	dbg_printf("write audio cost time %lu\n", (audio_end.tv_sec * 1000000 + audio_end.tv_usec) - (audio_start.tv_sec * 1000000 + audio_start.tv_usec));
#endif

	return  0;
}

int get_video(unsigned char *video_buf, unsigned int *video_size, unsigned int *video_time, int *is_key_frame, int stream_id)
{
	static int	serial_lock = -1;
	int	ret = 0;
	AV_DATA av_data;

	if(serial_lock < 0)
	{
		serial_lock = video_serial_start;
	}

	while(1)
	{
		ret = GetAVData(h264_field[LOCK_H264][stream_id], serial_lock, &av_data );
		
		if (ret == RET_SUCCESS)
		{
#ifdef WRITE_USER_HEADER
			memcpy(video_buf, av_data.ptr, av_data.size);
#else
			if(av_data.flags == AV_FLAGS_H264_I_FRAME)
			{
				memcpy(video_buf, av_data.ptr + 18, av_data.size - 18);
			}
			else
			{
				memcpy(video_buf, av_data.ptr + 6, av_data.size - 6);
			}
#endif

			GetAVData(h264_field[UNLOCK_H264][stream_id], serial_lock, &av_data);
	
			serial_lock++;	

			if(av_data.flags == AV_FLAGS_H264_I_FRAME)
			{
				*is_key_frame = 1;
			}
			else
			{
				*is_key_frame = 0;
			}

			*video_size = (int)av_data.size;
			*video_time = av_data.timestamp;

#ifdef WRITE_USER_HEADER
			if(*is_key_frame)
			{
				int sps_offset = 18;
				int pps_offset = sps_offset + 4 + sps_size;
				int nal_offset = pps_offset + 4 + pps_size;
				int nal_size = av_data.size - (pps_offset + 4 + pps_size) - 4;

				video_buf[0] = 0;
				video_buf[1] = 0;
				video_buf[2] = 0;
				video_buf[3] = 14;
				video_buf[sps_offset + 0] = 0;
				video_buf[sps_offset + 1] = 0;
				video_buf[sps_offset + 2] = 0;
				video_buf[sps_offset + 3] = sps_size;
				video_buf[pps_offset + 0] = 0;
				video_buf[pps_offset + 1] = 0;
				video_buf[pps_offset + 2] = 0;
				video_buf[pps_offset + 3] = pps_size;

				video_buf[nal_offset + 0] = (nal_size & 0xff000000) >> 24;
				video_buf[nal_offset + 1] = (nal_size & 0x00ff0000) >> 16;
				video_buf[nal_offset + 2] = (nal_size & 0x0000ff00) >> 8;
				video_buf[nal_offset + 3] = nal_size & 0x000000ff;

				dbg_printf("sps_offset %d pps_offset %d nal_offset %d nal_size %d %d %d %d\n",
						   sps_offset, pps_offset, nal_offset, nal_size,
						   video_buf[sps_offset + 4], video_buf[pps_offset + 4], video_buf[nal_offset + 4]);
			}
			else
			{
				video_buf[0] = 0;
				video_buf[1] = 0;
				video_buf[2] = 0;
				video_buf[3] = 2;
				int nal_size = av_data.size - 10;
				video_buf[6] = (nal_size & 0xff000000) >> 24;
				video_buf[7] = (nal_size & 0x00ff0000) >> 16;
				video_buf[8] = (nal_size & 0x0000ff00) >> 8;
				video_buf[9] = nal_size & 0x000000ff;
			}
#else
			if(*is_key_frame)
			{
				int sps_offset = 0;
				int pps_offset = sps_offset + 4 + sps_size;
				int nal_offset = pps_offset + 4 + pps_size;
				int nal_size = av_data.size - 18 - (pps_offset + 4 + pps_size) - 4;

				memmove(video_buf, video_buf + pps_offset + pps_size + 4, nal_size + 4);

				video_buf[0] = (nal_size & 0xff000000) >> 24;
				video_buf[1] = (nal_size & 0x00ff0000) >> 16;
				video_buf[2] = (nal_size & 0x0000ff00) >> 8;
				video_buf[3] = nal_size & 0x000000ff;

				dbg_printf("sps_offset %d pps_offset %d nal_offset %d nal_size %d %d %d %d\n",
						   sps_offset, pps_offset, nal_offset, nal_size,
						   video_buf[sps_offset + 4], video_buf[pps_offset + 4], video_buf[nal_offset + 4]);
			}
			else
			{
				int nal_size = av_data.size - 10;
				video_buf[0] = (nal_size & 0xff000000) >> 24;
				video_buf[1] = (nal_size & 0x00ff0000) >> 16;
				video_buf[2] = (nal_size & 0x0000ff00) >> 8;
				video_buf[3] = nal_size & 0x000000ff;
			}
#endif

			return 0;
		}
		else if (ret == RET_NO_VALID_DATA)
		{
			// dbg_printf("video lock %d no data\n", serial_lock);
			/* wait new frame */
			//GetAVData(h264_field[WAIT_NEW_H264_SERIAL][stream_id], -1, &av_data );
			//usleep(5);
			return -1;
		}
		else
		{
			GetAVData(h264_field[GET_H264_SERIAL][stream_id], -1, &av_data);
			serial_lock = av_data.serial;

			dbg_printf("video overwrite, get new serial %d\n", serial_lock);

			return -1;
		}

	}

	return 0;
}

static int write_video_frame(MP4FileHandle mp4_file_handle, MP4TrackId video_track, unsigned char *video_buf, unsigned int *video_size, unsigned int *video_time, int stream_id, int frame_rate)
{
	int ret = -1;
	int is_key_frame = 0;
	unsigned long long int duration = MP4_INVALID_DURATION;
	static int prev_video_time = 0;

	if(get_video(video_buf ,video_size, video_time, &is_key_frame, stream_id) < 0 )
	{
#ifdef RECORD_DEBUG
		dbg_printf("video no data!\n");
#endif
		return -1;
	}

#ifdef RECORD_DEBUG
	struct timeval video_start, video_end;
	gettimeofday(&video_start, NULL);
#endif

	if(prev_video_time == 0)
	{
		prev_video_time = *video_time;
	}
	else
	{
		duration = *video_time - prev_video_time;
		prev_video_time = *video_time;
	}
	/* dbg_printf("duration %d\n", *video_time - prev_video_time); */

	ret = MP4WriteSample(mp4_file_handle, video_track, video_buf, *video_size, duration, 0, is_key_frame);
	if(ret < 0)
	{
        dbg_printf( "write_frame failed, ret %d\n", ret);
        exit(1);
    }


#ifdef RECORD_DEBUG
	gettimeofday(&video_end, NULL);
	dbg_printf("write video cost time %lu\n", (video_end.tv_sec * 1000000 + video_end.tv_usec) - (video_start.tv_sec * 1000000 + video_start.tv_usec));
#endif

	return 0;
}

int send_snap_req(char *file_name, int file_num, const char *path)
{
	char snap_file_name[128];
	char snap_file_path[256];

	time_t time_now;
	struct tm tm;
	time(&time_now);
	gmtime_r(&time_now, &tm);

	if(file_name)
	{
		strcpy(snap_file_path, path);
		strcat(snap_file_path, strrchr(file_name, '/'));
		strcpy(snap_file_path + strlen(snap_file_path) - 3, "jpeg");
	}
	else
	{
		snprintf(snap_file_name, 128, "%04d_%04d%02d%02d%02d%02d%02d.jpeg",
				file_num, tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour,
				tm.tm_min, tm.tm_sec);
		snprintf(snap_file_path, 256, "%s/%s", path, snap_file_name);
	}

	//gen snap file
	msg_send_set_snap_req(SNAP_MODE_STREAM, snap_file_path);
	dbg_printf("snap_file_path %s\n", snap_file_path);
	//end

	return 0;
}

int main(int argc, char **argv)
{
    char *file_name;
	const char *interval_tmp;
	const char *file_num_tmp;
	const char *frame_rate_tmp;
	const char *is_audio_tmp;
	const char *is_chk_card_tmp;
	const char *stream_id_tmp;
	const char *reserve_space_tmp;
	const char *file_size_limit_tmp;

	int	msg_id = 0;
	int	sem_id = 0;
	int	file_num = 0;
	int	is_audio = 0;
	int	is_chk_card = 0;
	long long int reserve_space = 300 * 1024;
	long long int limit_space = 1800 * 1024;
	int stream_id = 0;
	int stream_duration = 60;
	int frame_rate = 30;
	int width = 1920;
	int height = 1080;

    if (argc != 10) {
        dbg_printf("usage: %s <output_file.mp4>  <interval>  <file_num> <frame_rate> <audio> <chkcard> <stream_id> <file_size_limit> <reserve_space>\n", argv[0]);
        exit(1);
    }

    file_name			= argv[1];
	interval_tmp		= argv[2];
	file_num_tmp 		= argv[3];
	frame_rate_tmp		= argv[4];
	is_audio_tmp		= argv[5];
	is_chk_card_tmp		= argv[6];
	stream_id_tmp		= argv[7];
	file_size_limit_tmp = argv[8];
	reserve_space_tmp	= argv[9];

	stream_duration 	= atoi(interval_tmp) * 1000 * 90;
	frame_rate			= atoi(frame_rate_tmp);
	file_num			= atoi(file_num_tmp);
	is_audio			= atoi(is_audio_tmp);
	is_chk_card			= atoi(is_chk_card_tmp);
	// stream_id		= atoi(stream_id_tmp);
	reserve_space		= (long long int )atoi(reserve_space_tmp) * 1024;
	limit_space			= (long long int )atol(file_size_limit_tmp) * 1024;

	dbg_printf( " file_name = %s interval = %d sec  frame_rate = %d file_num = %d \n  audio = %d chkcard = %d stream_id = %d limit_space = %lld reverse_space = %lld\n",
			file_name,
			stream_duration,
			frame_rate,
			file_num,
			is_audio,
			is_chk_card,
			stream_id,
			limit_space,
			reserve_space);

	curr_path = (is_chk_card == 1) ? MMC_PATH : TMP_PATH;

	if(check_reserve_space(is_chk_card, reserve_space, &limit_space) < 0)
	{
		return 100;
	}

	init_signals();
	msg_sem_shm_init(&msg_id, &sem_id);

	//snap
	int snap_cnt = 0;
	int snap_duration = stream_duration / SNAP_NUM;
	//end

#ifdef WRITE_SPS_PPS
	unsigned char sps[128] =
	{
		0x67, 0x4D, 0x40, 0x29, 0xE7, 0x40,	0x3C, 0x01, 0x13, 0xF2, 0xE0, 0x28, 0x10, 0x00, 0x00, 0x3E, 
		0x90, 0x00, 0x0E, 0xA6, 0x06, 0x00,	0x00, 0x03, 0x00, 0x2F, 0xAF, 0x08, 0x00, 0x00, 0x1D, 0xCD, 
		0x65, 0x3F, 0xFF, 0xE3, 0x00, 0x00,	0x03, 0x00, 0x17, 0xD7, 0x84, 0x00, 0x00, 0x0E, 0xE6, 0xB2, 
		0x9F, 0xFF, 0xF0, 0x20
	};

	unsigned char pps[12] = {0x68, 0xFE, 0x3C, 0x80};

	sleep(1);
	wait_video_start(stream_id, frame_rate);

	AV_DATA video_info;
	if(get_video_info(stream_id, &video_info) == 0)
	{
		int sps_offset = 0;
		int pps_offset = 0;
		sps_size = video_info.ptr[2];
		sps_offset = video_info.ptr[3];
		pps_size = video_info.ptr[4];
		pps_offset = video_info.ptr[5];

		memcpy(sps, (unsigned char *)video_info.ptr + sps_offset, sps_size);
		memcpy(pps, (unsigned char *)video_info.ptr + pps_offset, pps_size);

		width = video_info.width;
		height = video_info.height;

		dbg_printf("sps_size %d pps_size %d sps[0] %d pps[0] %d\n", sps_size, pps_size, sps[0], pps[0]);
	}
#endif

	dbg_printf("width %d height %d frame_rate %d\n", width, height, frame_rate);

	MP4FileHandle mp4_file_handle = MP4CreateEx(file_name, 0, 1, 1, 0, 0, 0, 0);//创建mp4文件
	if (mp4_file_handle == MP4_INVALID_FILE_HANDLE)
	{
		dbg_printf("open mp4_file_handle fialed.\n");
		return -1;
	}

	MP4SetTimeScale(mp4_file_handle, 90000);

	//添加h264 track    
	MP4TrackId video_track = MP4AddH264VideoTrack(mp4_file_handle, 90000, 90000 / 30, width, height,
			0x4D, //sps[1] AVCPromp4_file_handleIndication
			0x40, //sps[2] promp4_file_handle_compat
			0x29, //sps[3] AVCLevelIndication
			3); // 4 bytes length before each NAL unit
	if (video_track == MP4_INVALID_TRACK_ID)
	{
		dbg_printf("add video_track track failed.\n");
		MP4Close(mp4_file_handle, 0);    
		return -1;
	}

#ifdef WRITE_SPS_PPS
	MP4AddH264SequenceParameterSet(mp4_file_handle, video_track, sps, sps_size);
	MP4AddH264PictureParameterSet(mp4_file_handle, video_track, pps, pps_size);
#endif

	MP4SetVideoProfileLevel(mp4_file_handle, 0x7F);

	MP4TrackId audio_track;
	if(is_audio)
	{
		//添加aac音频
		audio_track = MP4AddAudioTrack(mp4_file_handle, 16000, 1024, MP4_MPEG4_AUDIO_TYPE);
		if (audio_track == MP4_INVALID_TRACK_ID)
		{
			dbg_printf("add audio_track track failed.\n");
			MP4Close(mp4_file_handle, 0);    
			return -1;
		}

#ifdef WRITE_AUDIO_SPEC_CONF
		short audio_specific_config = 0x0814;
		MP4SetTrackESConfiguration(mp4_file_handle, audio_track, (unsigned char *)&audio_specific_config, 2);
#endif
		MP4SetAudioProfileLevel(mp4_file_handle, 0x02);
	}

	unsigned char *video_buf = (unsigned char *)malloc(VIDEO_BUF_SIZE);
	unsigned char audio_buf[1024];
	int video_ret = -1;
	int audio_ret = -1;
	unsigned int audio_size = 0;
	unsigned int video_size = 0;
	unsigned int audio_pts = 0;
	unsigned int video_pts = 0;
	unsigned int audio_time = 0;
	unsigned int video_time = 0;
	unsigned int audio_basetime = 0;
	unsigned int video_basetime = 0;
	long long int file_size = 0;

    while(abort_signal == 0) 
	{
		video_ret = -1;
		audio_ret = -1;

		audio_pts = audio_time - audio_basetime;
		video_pts = video_time - video_basetime;

		if(file_size >= limit_space)
		{
			dbg_printf("limit_reach file_size %lld limit_space %lld\n", file_size, limit_space);
			if(reduce_limit_space_flag)
			{
				is_full = 1;
			}
			else
			{
				is_limit_reach = 1;
			}
			break;
		}

        if((audio_pts >= stream_duration) || (video_pts >= stream_duration))
		{
			dbg_printf( "pts %d is up %d %d\n", stream_duration, video_pts, audio_pts);
            break;
		}

#ifdef ENABLE_SNAP
		//snap
		if(snap_cnt < SNAP_NUM)
		{
			if((audio_pts > (snap_duration * snap_cnt)) || (video_pts > (snap_duration * snap_cnt)))
			{
				if(snap_cnt == 0)
				{
					send_snap_req(file_name, file_num, VIDEO_RECORD_THUMBNAIL_PATH);
				}
				else
				{
					send_snap_req(NULL, file_num, VIDEO_RECORD_THUMBNAIL_PATH);
				}
				snap_cnt++;
			}
		}
		//end
#endif

#ifdef RECORD_DEBUG
		dbg_printf("audio_pts %d video_pts %d\n", audio_pts, video_pts);
#endif

		while(!is_audio || (video_pts < audio_pts))
		{
			video_ret = write_video_frame(mp4_file_handle, video_track, video_buf, &video_size, &video_time, stream_id, frame_rate);
			if(video_ret < 0)
			{
				break;
			}
			video_pts = video_time - video_basetime;
			file_size += video_size;
		} 

		if(is_audio)
		{
			audio_ret = write_audio_frame(mp4_file_handle, audio_track, audio_buf, &audio_size, &audio_time);
			if(audio_ret >= 0)
			{
				file_size += audio_size;
			}
		}

		if(audio_basetime == 0)
		{
			audio_basetime = audio_time;
		}
		if(video_basetime == 0)
		{
			video_basetime = video_time;
		}

		if((video_ret < 0) && (audio_ret < 0))
		{
			usleep(10 * 1000);
		}
    }

	MP4Close(mp4_file_handle, 0);    

	if(is_limit_reach)
	{
		return 99;
	}
	else if(is_full)
	{
		return 100;
	}

	system("sync");

    return 0;
}

