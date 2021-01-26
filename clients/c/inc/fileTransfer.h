//--------------------------------------------------------------------------------------------------
/**
 * File transfer event types
 */
//--------------------------------------------------------------------------------------------------
enum fileTransferEventE
{
    FILETRANSFER_EVENT_INFO,                ///< Informational (ignored by ORP service)
    FILETRANSFER_EVENT_READY,               ///< Client ready for file transfer
    FILETRANSFER_EVENT_PENDING,             ///< Transfer pending
    FILETRANSFER_EVENT_START,               ///< File transfer start
    FILETRANSFER_EVENT_SUSPEND,             ///< File transfer suspend
    FILETRANSFER_EVENT_RESUME,              ///< File transfer resume
    FILETRANSFER_EVENT_COMPLETE,            ///< File transfer complete
    FILETRANSFER_EVENT_ABORT,               ///< File transfer aborted
};
