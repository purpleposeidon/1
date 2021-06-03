fn main() {
    // If a process ignores an FD sent as ancillary data, it gets closed.

    use nix::sys::socket;
    let (unix_a, unix_b) = socket::socketpair(
        socket::AddressFamily::Unix,
        socket::SockType::Stream,
        None,
        socket::SockFlag::empty(),
    ).unwrap();

    let (pipe_r, pipe_w) = nix::unistd::pipe().unwrap();

    trace!(nix::unistd::write(pipe_w, ":)".as_bytes()));

    let mut cmsg_buffer = nix::cmsg_space!(std::os::unix::io::RawFd);
    cmsg_buffer.clear();
    let iov_buff = "hey";
    let iov = nix::sys::uio::IoVec::from_slice(iov_buff.as_bytes());
    let msg = socket::sendmsg(
        unix_a,
        &[iov],
        &[
            socket::ControlMessage::ScmRights(&[pipe_r]),
        ],
        socket::MsgFlags::MSG_DONTWAIT,
        None,
    );
    trace!(msg.unwrap());
    nix::unistd::close(pipe_r);
    trace!(nix::unistd::write(pipe_w, "??".as_bytes()));
    let data = &mut [0; 8];
    trace!(nix::unistd::read(unix_b, data));
    trace!(data[..]);
    trace!(nix::unistd::write(pipe_w, "!!".as_bytes()));

    // 0) We make up an IOCTL and hope for a good kernel response:
    //      ENOTTY: Kernel doesn't know what we're talking about, use fallback
    //      EPROTO: Kernel lets us know the other end doesn't speak l1
    //      fileno: Kernel gives us a pipe to write on
    // 1) The pump *has* to be primed with a byte.
    //    This means that you can't begin with too much hidden output. :/
    //    (Or I could suck it up & buffer it all)
    //    (Or I could somehow convince the kernel to allow empty writes w/ ancillary messages...)
    // 2) Create a pipe; write a brief marker packet.
    // 3) Send the read end as ancillary data along with your first std-visible byte.
}

