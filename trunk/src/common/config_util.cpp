#include "config_util.h"

config_t* read_config_file ()
{
    Config *config = config_parse_default ();
    if (!config) {
        dbg (DBG_ERROR, "Could not reach config file.");
        return NULL;
    }

    config_t *cfg = (config_t*)malloc(sizeof(config_t));

    int status=0;
    
    // read the system platform
    char *platform = (char*)malloc(10*sizeof(char));
    status = config_get_str (config, "platform", &platform);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read config file.");
        return NULL;
    }

    if (strncmp (platform, "backpack", 8) == 0) {
        cfg->platform = PLATFORM_BACKPACK;
    } else if (strncmp (platform, "dgc", 3) == 0) {
        cfg->platform = PLATFORM_DGC;
    } else {
        dbg (DBG_ERROR, "unknown platform: %s", platform);
        return NULL;
    }

    // read the number of sensors
    char key[60];
    sprintf (key, "cameras.%s.nsensors", platform);
    status = config_get_int (config, key, &cfg->nsensors);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key %s", key);
        return NULL;
    }

    // read the number of buckets (classifier)
    status = config_get_int (config, "classifier.nbuckets", &cfg->nbuckets);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key classifier.nbuckets");
        return NULL;
    }

    // read the channel names
    sprintf (key, "cameras.%s.channels", platform);
    char **channels = config_get_str_array_alloc(config, key);

    if (!channels) {
        dbg (DBG_ERROR, "failed to read key %s", key);
        return NULL;
    }

    cfg->channel_names = channels;

    // read the unit IDs (dc1394)
    sprintf (key, "cameras.%s.unit_ids", platform);
    char **unit_ids = config_get_str_array_alloc (config, key);

    if (!unit_ids) {
        dbg (DBG_ERROR, "failed to read key %s", key);
        return NULL;
    }

    cfg->unit_ids = unit_ids;

    // read the data directory
    char *data_dir = (char*)malloc(256);
    status = config_get_str (config, "data_dir", &data_dir);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key: data_dir");
        return NULL;
    }
    cfg->data_dir = data_dir;


    // read the features max delay in sec
    status = config_get_int (config, "features.max_delay_sec", &cfg->features_max_delay_sec);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.max_delay_sec.");
        return NULL;
    }

    status = config_get_double (config, "tracker.max_dist", &cfg->tracker_max_dist);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key tracker.max_dist");
        return NULL;
    }

    status = config_get_double (config, "tracker.add_min_dist", &cfg->tracker_add_min_dist);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key tracker.add_min_dist");
        return NULL;
    }

    status = config_get_double (config, "tracker.second_neighbor_ratio", &cfg->tracker_second_neighbor_ratio);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key tracker.second_neighbor_ratio");
        return NULL;
    }


    status = config_get_double (config, "features.scale_factor", &cfg->features_scale_factor);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.scale_factor");
        return NULL;
    }

    char *feature_type = (char*)malloc(50);
    status = config_get_str (config, "features.feature_type", &feature_type);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.feature_type");
        return NULL;
    }
    if (strcmp (feature_type, "SIFT") == 0) {
        cfg->features_feature_type = NAVLCM_FEATURES_PARAM_T_SIFT;
    } else if (strcmp (feature_type, "FAST") == 0) {
        cfg->features_feature_type = NAVLCM_FEATURES_PARAM_T_FAST;
    } else if (strcmp (feature_type, "SURF64") == 0) {
        cfg->features_feature_type = NAVLCM_FEATURES_PARAM_T_SURF64;
    } else if (strcmp (feature_type, "SURF128") == 0) {
        cfg->features_feature_type = NAVLCM_FEATURES_PARAM_T_SURF128;
    } else if (strcmp (feature_type, "GFTT") == 0) {
        cfg->features_feature_type = NAVLCM_FEATURES_PARAM_T_GFTT;
    } else if (strcmp (feature_type, "SIFT2") == 0) {
        cfg->features_feature_type = NAVLCM_FEATURES_PARAM_T_SIFT2;
    } else {
        dbg (DBG_ERROR, "unrecognized feature type %s", feature_type);
        return NULL;
    }
    free (feature_type);

    // sift parameters
    status = config_get_boolean (config, "features.sift_doubleimsize", &cfg->sift_doubleimsize);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.sift_doubleimsize");
        return NULL;
    }
    status = config_get_int (config, "features.sift_nscales", &cfg->sift_nscales);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.sift_nscales");
        return NULL;
    }
    status = config_get_double (config, "features.sift_sigma", &cfg->sift_sigma);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.sift_sigma");
        return NULL;
    }
    status = config_get_double (config, "features.sift_peakthresh", &cfg->sift_peakthresh);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.sift_peakthresh");
        return NULL;
    }

    // gftt parameters
    status = config_get_double (config, "features.gftt_quality", &cfg->gftt_quality);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.gftt_quality");
        return NULL;
    }
    status = config_get_double (config, "features.gftt_min_dist", &cfg->gftt_min_dist);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.gftt_min_dist");
        return NULL;
    }
    status = config_get_int (config, "features.gftt_block_size", &cfg->gftt_block_size);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.gftt_block_size");
        return NULL;
    }
    status = config_get_boolean (config, "features.gftt_use_harris", &cfg->gftt_use_harris);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.gftt_use_harris");
        return NULL;
    }
    status = config_get_double (config, "features.gftt_harris_param", &cfg->gftt_harris_param);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.gftt_harris_param");
        return NULL;
    }

    // surf parameters
    status = config_get_int (config, "features.surf_octaves",&cfg->surf_octaves);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.surf_octaves");
        return NULL;
    }
    status = config_get_int (config, "features.surf_intervals",&cfg->surf_intervals);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.surf_intervals");
        return NULL;
    }
    status = config_get_int (config, "features.surf_init_sample",&cfg->surf_init_sample);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.surf_init_sample");
        return NULL;
    }
    status = config_get_double (config, "features.surf_threshold",&cfg->surf_threshold);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.surf_threshold");
        return NULL;
    }

    // fast parameters
    status = config_get_int (config, "features.fast_threshold",&cfg->fast_threshold);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key features.fast_threshold");
        return NULL;
    }

    // classifier calibration filename 
    cfg->classcalib_filename = (char*)malloc(256);
    status = config_get_str (config, "classifier.calib_file", &cfg->classcalib_filename);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key classifier.calib_file");
        return NULL;
    }

    cfg->classcalib = NULL;

    // read the upward camera calibration
    status = config_get_double_array (config, "cameras.upward_calib.fc", cfg->upward_calib_fc, 2);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key cameras.upward_calib.fc");
        return NULL;
    }

    status = config_get_double_array (config, "cameras.upward_calib.cc", cfg->upward_calib_cc, 2);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key cameras.upward_calib.cc");
        return NULL;
    }
    status = config_get_double_array (config, "cameras.upward_calib.kc", cfg->upward_calib_kc, 5);
    if (status < 0) {
        dbg (DBG_ERROR, "failed to read key cameras.upward_calib.kc");
        return NULL;
    }
    free (platform);

    return cfg;
}
