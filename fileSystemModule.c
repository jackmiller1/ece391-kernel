#import "fileSystemModule.h"
#include "system_calls.h" //for PCB struct
#import "lib.h"
#import "types.h"

//file scope vars
static int directoryLoc = 0;

/*
*   Function: read_dentry_by_name
*   Description: given a filename and a dentry struct to fill, this funciton finds the corresponding dentry in
*                the filesystem (if it exists) and fills the given dentry with the fileName, fileType, and 
*                inodeNumber
*   inputs: fname: a c string containing the name of the file we are searching for
*   outputs: dentry: a dentry struct * that will be filled with info about the dentry specified in fname
*   returns: 0 on success, -1 on failure
*/
int32_t read_dentry_by_name (const uint8_t* fname, dentry_t* dentry)
{
    unsigned int * fileSystemStart = (unsigned int *)FILESYSLOC;
    unsigned int numDirectories = *fileSystemStart;
    int i;
    int curLen;
    //check for valid input
    int length = strlen((int8_t*)fname);

    if (length > NAMESIZE) {
        return -1;
    }
    for (i = 0; i < numDirectories; i++)
    {
        //calculate address of directory entry
        unsigned int * directory = (unsigned int *)(FILESYSLOC + (i+1)*DENTRYSIZE);
        //get name of current file
        int8_t curName[33];
        strncpy((int8_t*)curName, (int8_t*)directory, NAMESIZE);
        curName[NAMESIZE] = '\0';
        //get size of name of current file
        curLen = strlen((int8_t*)(curName));
        //if we found it, copy data and return
        if (length == curLen && strncmp((int8_t*)fname, (int8_t*)curName, length) == 0)
        {
            strncpy((int8_t*)(dentry->fileName), (int8_t*)curName, NAMESIZE);
            dentry->fileName[NAMESIZE] = '\0';
            dentry->fileType = *((unsigned int *)((unsigned int)directory + NAMESIZE));
            dentry->inodeNumber = *((unsigned int *)((unsigned int)directory + NAMESIZE + sizeof(unsigned int)));
            return 0;
        }
    }
    return -1;
}


/*
 *   Function: read_dentry_by_index
 *   Description: given an index into the bootblock dir entires, and a dentry struct to fill, 
 *                this funciton finds the corresponding dentry in
 *                the filesystem (if it exists) and fills the given dentry with the fileName, fileType, and
 *                inodeNumber
 *   inputs: index: an index into the bootblock dir entires
 *   outputs: dentry: a dentry struct * that will be filled with info about the dentry specified in fname
 *   returns: 0 on success, -1 on failure
 */
int32_t read_dentry_by_index (uint32_t index, dentry_t* dentry)
{
    unsigned int * fileSystemStart = (unsigned int *)FILESYSLOC;
    unsigned int numDirectories = *fileSystemStart;
    //check for valid index
    if (index < 0 || index >= numDirectories) {
        return -1;
    }
    //calculate address of directory entry
    unsigned int * directory = (unsigned int *)(FILESYSLOC + (index +1)*DENTRYSIZE);
    
    strncpy((int8_t*)(dentry->fileName), (int8_t*)directory, NAMESIZE);
    dentry->fileName[NAMESIZE] = '\0';
    dentry->fileType = *((unsigned int *)((unsigned int)directory + NAMESIZE));
    dentry->inodeNumber = *((unsigned int *)((unsigned int)directory + NAMESIZE + sizeof(unsigned int)));
    return 0;
}

/*
*   Function: read_data()
*   Description: Helper function that reads bytes in the file starting from a given 
*                offset and copies the bytes read into a buffer
*   inputs: inode -- the inode number of the file to read from
*           offset -- the position in the file (in bytes) from which to start the read from
*           buf -- pointer to the buffer to copy data into
*           length -- the number of bytes to read
*   outputs: Returns the number of bytes read, or -1 on failure
*   effects: copys data from a file into buf
*/
int32_t read_data (uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
{
    uint32_t byte_count = 0;
    uint8_t * boot_block_ptr = (uint8_t *)FILESYSLOC;
    uint32_t total_inodes = *((uint32_t *)(boot_block_ptr + INODE_BYTE_OFFSET)); //get the total number of inodes

    if (inode < 0 || inode >= total_inodes) // return -1 if inode number is invalid
        return -1;

    uint32_t total_data_blocks = *((uint32_t *)(boot_block_ptr + DATA_BLOCK_BYTE_OFFSET)); //get the total number of data blocks
    uint8_t * inode_ptr = boot_block_ptr + BLOCK_SIZE*(inode + 1); //set a pointer to the relevant inode
    uint8_t * data_block_start_ptr = boot_block_ptr + BLOCK_SIZE*(total_inodes + 1); //set a pointer to the start of the data blocks
    uint32_t file_size = *((uint32_t *)(inode_ptr)); //get the file size in bytes

    if (offset >= file_size)//return 0 if offset is at or past the end of file
        return 0;
    
    if (length + offset > file_size)// if more bytes are requested than available, cut the number of bytes requested
        length = file_size - offset;
	

    //set a pointer to the first relevant data block index in inode
    uint32_t * data_block_index_ptr_in_inode = (uint32_t *)(inode_ptr + (offset/BLOCK_SIZE + 1)*INODE_BYTE_OFFSET);
    if (*data_block_index_ptr_in_inode < 0 || *data_block_index_ptr_in_inode >= total_data_blocks) //return -1 if data block number is invalid
        return -1;
    
    //handle the first data block
    uint8_t * data_ptr = data_block_start_ptr + BLOCK_SIZE*(*data_block_index_ptr_in_inode) + offset%BLOCK_SIZE;//set pointer to start of data to copy
    if (offset%BLOCK_SIZE + length <= BLOCK_SIZE )
    {
        memcpy(buf, data_ptr, length);//copy length bytes into buf
        byte_count += length;
        return byte_count;
    }
    else
    {
        //copy (BLOCK_SIZE - offset) bytes into buf and change length and buf accordingly
        memcpy(buf, data_ptr, BLOCK_SIZE - (offset%BLOCK_SIZE));
        length -= BLOCK_SIZE - (offset%BLOCK_SIZE);
        buf += BLOCK_SIZE - (offset%BLOCK_SIZE);
        byte_count += BLOCK_SIZE - (offset%BLOCK_SIZE);
    }

    //process the rest of the data blocks
    while (length > 0)
    {
        data_block_index_ptr_in_inode++; //increment data_block_index ptr
        if (*data_block_index_ptr_in_inode < 0 || *data_block_index_ptr_in_inode >= total_data_blocks) //return -1 if data block number is invalid
        {
            return -1;
        }
        data_ptr = data_block_start_ptr + BLOCK_SIZE*(*(data_block_index_ptr_in_inode));
        if (length <= BLOCK_SIZE)
        {
            memcpy(buf, data_ptr, length);//copy length bytes into buf
            byte_count += length;
            return byte_count;
        }
        else
        {
            //copy BLOCK_SIZE bytes into buf and change length and buf accordingly
            memcpy(buf, data_ptr, BLOCK_SIZE);
            length -= BLOCK_SIZE;
            buf += BLOCK_SIZE;
            byte_count += BLOCK_SIZE;
        }
    }
    return 0;
}

/*
*   Function: file_open()
*   Description: opens a file
*   inputs: filename -- name of the file to open
*   outputs: return 0 on success
*   effects: 
*/
int32_t file_open (const uint8_t* filename)
{
    return 0;
}

/*
*   Function: file_read()
*   Description: Reads bytes from a file and copies the bytes read into a buffer
*   inputs: fd -- file descriptor of the file to read
*           buf -- pointer to the buffer to copy data into
*           nbytes -- number of bytes to read
*   outputs: Returns the number of bytes read, or -1 on failure 
*   effects: copys data from a file into buf
*/
int32_t file_read (int32_t fd, void* buf, int32_t nbytes)
{
    uint32_t inode_number;

    /* Get current PCB Pointer */
    pcb_t *pcb = get_pcb_ptr();

    //get current file position
    uint32_t offset = pcb->fds[fd].file_position;
    //search for the file by name
    inode_number = pcb->fds[fd].inode;
    int temp = read_data(inode_number, offset, (uint8_t*)buf, nbytes);
    
    pcb->fds[fd].file_position += temp;
    
    return temp;
}

/*
*   Function: file_write()
*   Description: writes to a file
*   inputs: 
*   outputs: return -1 on failure
*   effects: always fails because this is read only file system
*/
int32_t file_write (int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

/*
*   Function: file_close()
*   Description: closes the file
*   inputs: fd -- file descriptor for the file to close
*   outputs: return 0 on success
*   effects: 
*/
int32_t file_close (int32_t fd)
{
    return 0;
}

/*
*   Function: dir_open()
*   Description: opens a directory
*   inputs: filename -- name of the file to open
*   outputs: return 0 on success
*   effects: 
*/
int32_t dir_open (const uint8_t* filename)
{
    return 0;
}

/*
*   Function: dir_read
*   Description: this functions outputs (into buf) the name of the file or directory specified in the
*                directoryLoc file variable, which is then incremented (or returned to 0 if it passes the end)
*                the inputs fd and nbytes are ignored.
*   inputs: fd and nbytes are ignored
*   outputs: buf is filled with a c string (not null terminated) containing the name of the file
*   returns: number of bytes written (will be 0 if we've reached the end)
*/
int32_t dir_read (int32_t fd, void* buf, int32_t nbytes)
{
    dentry_t dentry;
    int i;
	
    if (read_dentry_by_index(directoryLoc, &dentry) == 0)
    {
        //clear buffer
        for (i = 0; i < 33; i++)
            ((int8_t*)(buf))[i] = '\0';

        //copy file name into buffer if directory entry can be read
        int32_t len = strlen((int8_t*)dentry.fileName);
        strncpy((int8_t*)buf, (int8_t*)dentry.fileName, len);
        directoryLoc++;
        return len;
    }
    else
    {
		/*TEMP FIX = directoryLoc needs to be changed for checkpoint 3 */
		directoryLoc = 0;
        return 0;
    }
}

/*
*   Function: dir_write()
*   Description: writes to a directory
*   inputs: 
*   outputs: return -1 on failure
*   effects: always fails 
*/
int32_t dir_write (int32_t fd, const void* buf, int32_t nbytes)
{
    return -1;
}

/*
*   Function: dir_close()
*   Description: closes the directory
*   inputs: fd -- file descriptor for the directory to close
*   outputs: return 0 on success
*   effects: 
*/
int32_t dir_close (int32_t fd)
{
    return 0;
}
