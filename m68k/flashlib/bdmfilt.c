#include <bdmfilt.h>
#include <errno.h>
#include <string.h>

int bdmlib_load_use_lma=0;

bdmlib_bfilt_t * bdmlib_bfilt=NULL;

int
bdmfilt_wrb_filt(bdmlib_bfilt_t * filt, caddr_t in_adr,
		u_int size, u_char * bl_ptr)
{
	int ret, part_ret;
	u_int part_size;	
	if (!size) return 0;

	while (filt) {
		if((in_adr<=filt->end_adr)&&
		   (in_adr+size-1>=filt->begin_adr)) {
			ret = 0;
			if(in_adr<filt->begin_adr) {
				part_size=filt->begin_adr-in_adr;
				part_ret=bdmlib_wrb_filt(filt->next,in_adr,part_size,bl_ptr);
				ret+=part_ret;
				if(part_ret!=part_size) return ret;
				in_adr+=part_size;
				bl_ptr+=part_size;
				size-=part_size;
			}
			part_size=filt->end_adr-in_adr+1;
			if (part_size>=size) {
				part_size=size; 
				size=0;
			} else size-=part_size;
			part_ret=filt->wrb_filt(filt,in_adr,part_size,bl_ptr);
			ret+=part_ret;
			if(part_ret!=part_size) {
				dbprintf("write error on filt write, address 0x%lx size %d acknowledged %d errno %d\n",
						(long)in_adr, part_size, part_ret, errno);
				return ret;
			}
			in_adr+=part_size;
			bl_ptr+=part_size;
			if(!size) return ret;
			ret+=bdmlib_wrb_filt(filt->next,in_adr,size,bl_ptr);
			return ret;
		}
		filt=filt->next;
	}
	/* regular memory block write */
	if ((ret = bdmlib_write_block(in_adr, size, bl_ptr)) != size) {
		dbprintf("write error on block write, written %d acknowledged %d errno %d\n",
				size, ret, errno);
		return ret;
	}
	return ret;
}

