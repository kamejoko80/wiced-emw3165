/*
 * Copyright 2015, Broadcom Corporation
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */
/* $Id: audioProcessing.h 1.5 2011/03/22 21:39:06 rzopf Exp $ */
/* $Log: audioProcessing.h $
 * Revision 1.5  2011/03/22 21:39:06  rzopf
 * code cleanup.
 *
 * Revision 1.4  2011/03/15 20:57:23  rzopf
 * SBC stereo.
 *
 * Revision 1.3  2011/03/11 20:27:15  rzopf
 * stereo support
 *
 * Revision 1.2  2011/03/10 18:20:39  rzopf
 * AAC functionality
 *
 * Revision 1.1  2011/02/24 18:03:02  rzopf
 * Initial Version.
 *
 * */

/* Author: Rob Zopf                                           */
/* January 6, 2011                                            */

#ifndef AUDIOPROCESSING_H
#define AUDIOPROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif

#define GOOD_FRAME   0
#define BAD_FRAME    1

#define  CODEC_PCM      0
#define  CODEC_CVSD     1
#define  CODEC_SBC      2
#define  CODEC_AACLC    3

#define MAXFLSBC        128
#define MAXFLAAC        1024

/*************************************************************************************
 * Function audioDecoding()
 *
 * Purpose:
 *
 * Inputs:
 *
 *
 * Outputs
 *
 * Return   -
 *
 *************************************************************************************/
void audioDecoding(void *CodecMemHandle, struct AUDIOPLC_STRUCT *aplc, int CodecType, short *indices, int FrameType, int frmsz,
                   short *outbuf, int *aSrcUsed, int *aOutDstLen, int nChan);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AUDIOPROCESSING_H */



