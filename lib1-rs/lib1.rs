// FIXME: much unsafe; use more crate:nix

use std::{mem, io, fmt};
use std::os::unix::prelude::*;
use std::convert::TryInto;
use std::fs::File;

pub static DEV_PATH: &str = "/dev/1";
pub const IOCTL_WRAP: u64 = 0x40043100;
pub const IOCTL_UNWRAP: u64 = 0x3101;
//pub const IOCTL_MARK_STEM: u64 = 0x40043102;
//pub const IOCTL_FIND_STEM: u64 = 0x3103;
// See [L1_IOCTL_DEFS]
pub static STEM_FD: &str = "LIB1_STEM_FD";
pub const STEM_IOV_PAYLOAD: u8 = b'1';



#[repr(packed(1))]
pub struct Header {
    msg_len: libc::c_ushort,
    version: libc::c_uchar,
    flags: libc::c_char,
    ty: libc::c_char,
}
impl Header {
    pub const fn zeroed() -> Self {
        Header {
            msg_len: 0,
            version: 0,
            flags: 0,
            ty: 0,
        }
    }
    pub fn as_bytes<'a>(&'a self) -> &'a [u8] {
        unsafe {
            std::slice::from_raw_parts(
                (self as *const Self) as *const u8,
                mem::size_of::<Self>(),
            )
        }
    }
    pub fn as_mut_bytes<'a>(&'a mut self) -> &'a mut [u8] {
        unsafe {
            std::slice::from_raw_parts_mut(
                (self as *mut Self) as *mut u8,
                mem::size_of::<Self>(),
            )
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Flags(pub i8);
pub const F_STD_VISIBLE: i8 = 0x1;
pub const F_INCOMPLETE: i8 = 0x2;
pub const F_SYNTHETIC: i8 = 0x4; // This packet came from an old program writing to a wrapped file.
impl Flags {
    #[inline]
    pub fn std_visible(&self) -> bool {
        (self.0 & F_STD_VISIBLE) != 0
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash)]
pub struct Type(pub i8);
// FIXME: See other EACH_L1_HEADER_TYPE
// Can't be an enum 'cuz we might get a weird one
pub const PUSH: Type = Type('(' as i8);
pub const POP: Type = Type(')' as i8);
pub const FIELD_NAME: Type = Type('$' as i8);
pub const FIELD_TYPE: Type = Type(':' as i8);
pub const FIELD_VALUE: Type = Type('=' as i8);
pub const FIELD_TEXT: Type = Type('>' as i8);
pub const FORMAT: Type = Type('-' as i8);
pub const COMMENT: Type = Type('_' as i8);
pub const LABEL: Type = Type('*' as i8);
impl Type {
    pub const fn default_flags(&self) -> Flags {
        if self.std_visible() {
            Flags(F_STD_VISIBLE)
        } else {
            Flags(0)
        }
    }
    pub const fn std_visible(self) -> bool {
        match self {
            // EACH_L1_HEADER_TYPE
            PUSH | POP | FIELD_NAME | FIELD_TYPE | COMMENT | LABEL | FIELD_VALUE => false,
            FIELD_TEXT | FORMAT => true,
            _ => true,
        }
    }
    pub fn close_pair(self) -> Type {
        match self {
            PUSH => POP,
            _ => panic!("packet type {:?} doesn't have a closing pair", self),
        }
    }
}

#[derive(Copy, Clone)]
pub struct Packet<'a> {
    pub flags: Flags,
    pub ty: Type,
    pub msg: &'a [u8],
}
impl<'a> fmt::Debug for Packet<'a> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        if self.flags != self.ty.default_flags() {
            write!(f, "{:?}", self.flags)?;
        }
        // FIXME: format self.ty more carefully
        if let Ok(s) = std::str::from_utf8(self.msg) {
            write!(f, "{}{:?}", self.ty.0 as u8 as char, s)
        } else {
            write!(f, "{}{:?}", self.ty.0 as u8 as char, self.msg)
        }
    }
}
impl<'a> Packet<'a> {
    pub fn header(&self) -> Header {
        Header {
            msg_len: self.msg.len().try_into().unwrap(),
            version: VERSION,
            flags: self.flags.0,
            ty: self.ty.0,
        }
    }
    pub fn is_empty(&self) -> bool {
        self.msg.is_empty()
    }
    pub fn len(&self) -> usize {
        self.msg.len()
    }
}

pub const PRELUDE_LEN: usize = mem::size_of::<libc::c_ushort>() + mem::size_of::<libc::c_uchar>();
pub const VERSION: u8 = (mem::size_of::<Header>() - PRELUDE_LEN) as u8;


#[derive(Debug)]
pub struct Atr {
    file: File,
    status: bool,
}
#[cfg(unix)]
impl std::os::unix::io::AsRawFd for Atr {
    fn as_raw_fd(&self) -> RawFd { self.file.as_raw_fd() }
}
#[cfg(unix)]
impl std::os::unix::io::IntoRawFd for Atr {
    fn into_raw_fd(self) -> RawFd { self.file.into_raw_fd() }
}
impl Atr {
    #[cfg(unix)]
    pub fn connect(file: File) -> io::Result<Self> {
        unsafe {
            Self::connect_raw(file.into_raw_fd())
        }
    }
    #[cfg(unix)]
    pub unsafe fn connect_tty_stdout() -> io::Result<Option<Self>> {
        fn e<T>(r: Result<T, nix::Error>) -> io::Result<T> {
            match r {
                Ok(r) => Ok(r),
                Err(nix::Error::Sys(e)) => Err(e.into()),
                Err(e) => panic!("{}", e),
            }
        }
        if !e(nix::unistd::isatty(libc::STDOUT_FILENO))? { return Ok(None); }
        let stem_var = match std::env::var(STEM_FD) {
            Err(std::env::VarError::NotPresent) => return Ok(None),
            Err(e) => panic!("{}", e),
            Ok(v) => v,
        };
        let stem_fd = match stem_var.parse() {
            Ok(fd) => fd,
            Err(_) => panic!("not a valid fd: {:?}", stem_var),
        };
        let stem_fd = File::from_raw_fd(stem_fd);
        let (read_end, write_end) = e(nix::unistd::pipe())?;
        let payload = &[STEM_IOV_PAYLOAD][..];
        let e = {
            use nix::sys::socket;
            use nix::sys::uio::IoVec;
            let iov = IoVec::from_slice(payload);
            socket::sendmsg(
                stem_fd.as_raw_fd(),
                &[iov],
                &[socket::ControlMessage::ScmRights(&[read_end])],
                socket::MsgFlags::empty(),
                None,
            )
        };
        if e != Ok(payload.len()) {
            panic!("budding failed");
        }
        nix::unistd::close(read_end).ok();
        let atr = File::from_raw_fd(write_end);
        return Ok(Some(Atr::from_atr(atr)));
    }
    #[cfg(unix)]
    pub unsafe fn connect_raw(wrapped: RawFd) -> io::Result<Self> {
        if wrapped == libc::STDOUT_FILENO {
            if let Some(a) = Self::connect_tty_stdout()? {
                return Ok(a);
            }
        }
        let naked = libc::ioctl(wrapped, IOCTL_UNWRAP);
        let file = File::from_raw_fd(naked);
        if naked < 0 {
            if libc::ENOTTY == *libc::__errno_location() {
                let file = File::from_raw_fd(wrapped);
                // FIXME: Think about this in regards to File's correctness
                eprintln!("connect_raw: wasn't wrapped: {:?}", file);
                Ok(Atr { file, status: false })
            } else {
                eprintln!("connect_raw: error");
                Err(io::Error::last_os_error())
            }
        } else {
            eprintln!("connect_raw: OK: {:?}", file);
            Ok(Atr { file, status: true })
        }
    }
    #[cfg(unix)]
    pub unsafe fn from_std(file: File) -> Self {
        Atr {
            file,
            status: false,
        }
    }
    #[cfg(unix)]
    pub unsafe fn from_atr(file: File) -> Self {
        Atr {
            file,
            status: true,
        }
    }
    pub fn write(&mut self, p: &Packet) -> io::Result<usize> {
        unsafe {
            if self.status {
                let header = p.header();
                let header = header.as_bytes();
                let mut buf = Vec::with_capacity(header.len() + p.msg.len());
                buf.extend(header.iter().cloned());
                buf.extend(p.msg.iter().cloned());
                let n = libc::write(
                    self.file.as_raw_fd(),
                    buf.as_ptr() as *const libc::c_void,
                    buf.len(),
                );
                if n < 0 {
                    return Err(io::Error::last_os_error());
                } else if n != buf.len() as libc::ssize_t && n != 0 {
                    // FIXME: What's the right way to handle this?
                    panic!("write failed");
                }
                if p.flags.std_visible() {
                    Ok(p.msg.len())
                } else {
                    Ok(0)
                }
            } else if p.flags.std_visible() {
                let n = libc::write(
                    self.file.as_raw_fd(),
                    p.msg.as_ptr() as *const libc::c_void,
                    p.msg.len(),
                );
                if n < 0 {
                    return Err(io::Error::last_os_error());
                };
                let n = n as usize;
                if n != p.msg.len() {
                    panic!("write failed");
                }
                Ok(n)
            } else {
                Ok(0)
            }
        }
    }
    pub fn read<'a>(&mut self, buf: &'a mut Vec<u8>) -> io::Result<Packet<'a>> {
        buf.clear();
        if !self.status {
                if buf.capacity() < 0x100 {
                    buf.reserve(0x100);
                }
                let n = unsafe {
                    libc::read(
                        self.file.as_raw_fd(),
                        buf.as_mut_ptr() as *mut libc::c_void,
                        buf.capacity(),
                    )
                };
                if n < 0 {
                    // FIXME: read into header, just like the kernel impl
                    eprintln!("  n < 0: n == {}", n);
                    return Err(io::Error::last_os_error());
                }
                unsafe { buf.set_len(n as usize); }
                let ty = FORMAT;
                return Ok(Packet {
                    flags: ty.default_flags(),
                    ty,
                    msg: buf.as_slice(),
                });
        } else {
            let mut header = Header::zeroed();
            let prelude = std::mem::size_of::<libc::c_ushort>() + std::mem::size_of::<libc::c_uchar>();
            let n = unsafe {
                libc::read(
                    self.file.as_raw_fd(),
                    header.as_mut_bytes().as_mut_ptr() as *mut libc::c_void,
                    prelude,
                )
            };
            if n < 0 {
                eprintln!("  header fail: n = {}", n);
                return Err(io::Error::last_os_error());
            }
            if n == 0 {
                return Ok(Packet {
                    flags: Flags(F_SYNTHETIC | F_STD_VISIBLE),
                    ty: LABEL,
                    msg: &[],
                });
            }
            let n = n as usize;
            if n != prelude {
                eprintln!("  prelude fail: n = {}", n);
                return Err(io::Error::from_raw_os_error(libc::EPROTO));
            }
            if header.version != VERSION {
                panic!("version mismatch: actual {}, expected {}", header.version, VERSION); // FIXME
            }
            let n = unsafe {
                libc::read(
                    self.file.as_raw_fd(),
                    &mut header.flags as *mut i8 as *mut libc::c_void,
                    VERSION as libc::size_t,
                )
            };
            if n < 0 {
                eprintln!("  read fail: n = {}", n);
                return Err(io::Error::last_os_error());
            } else if n != VERSION as _ {
                eprintln!("  read VERSION fail: n = {}", n);
                return Err(io::Error::from_raw_os_error(libc::EPROTO));
            }
            buf.reserve(header.msg_len as usize);
            let n = unsafe {
                libc::read(
                    self.file.as_raw_fd(),
                    buf.as_mut_ptr() as *mut libc::c_void,
                    header.msg_len as usize,
                )
            };
            if n < 0 {
                eprintln!("  read buf fail: n = {}", n);
                return Err(io::Error::last_os_error());
            }
            let n = n as usize;
            if n != header.msg_len.try_into().unwrap() {
                {
                    let msg_len = header.msg_len;
                    eprintln!("  read buf incomplete!: n = {}, msg_len = {}", n, msg_len);
                }
                // FIXME: Uhm... keep reading I guess?
                return Err(io::Error::from_raw_os_error(libc::EPROTO));
            }
            unsafe { buf.set_len(n); }
            let p = Packet {
                flags: Flags(header.flags),
                ty: Type(header.ty),
                msg: buf.as_slice(),
            };
            Ok(p)
        }
    }
    pub fn as_naked(&mut self) -> &mut File { &mut self.file }
    pub fn into_reader(self) -> ReadAtr {
        ReadAtr {
            atr: self,
            in_buf: vec![],
            l: 0,
        }
    }
    pub fn into_writer(self) -> WriteAtr {
        WriteAtr(self)
    }
}

pub fn wrap(naked: RawFd, oflag: libc::c_int) -> Result<RawFd, io::Error> {
    use std::ffi::CStr;
    unsafe {
        let flags = libc::fcntl(naked, libc::F_GETFD);
        if flags < 0 {
            return Err(io::Error::last_os_error());
        }
        let wrapper = libc::open(CStr::from_bytes_with_nul(b"/dev/1\0").unwrap().as_ptr(), oflag);
        if wrapper < 0 {
            return Err(io::Error::last_os_error());
        }
        let err = libc::ioctl(wrapper, IOCTL_WRAP, naked);
        if err < 0 {
            return Err(io::Error::last_os_error());
        }
        Ok(wrapper)
    }
}

#[test]
fn check_version() {
    assert_eq!(mem::size_of::<Header>(), 5);
    assert_eq!(VERSION, 2);
}

/// ```
/// let cheese_info = lib1::fmt! {
///     LABEL:"favorite food",
///     PUSH:"1" => {
///         FIELD_TYPE:"food",
///         FIELD_TEXT:"cheese",
///     },
/// };
/// let mut stdout = lib1::stdout();
/// cheese_info.write(&mut stdout).unwrap();
/// ```
#[macro_export]
macro_rules! fmt {
    // FIXME: Rewrite into a much more efficient procedural macro.
    // Macro-rules seems like it would be hard/impossible/shitty-try-hard.
    (
        $(
            $packet_type:ident : $packet_value:expr
            $(=> { $($deliminated:tt)* })?
        ),* $(,)*
    ) => {{{
        let mut fmt = $crate::Format {
            types: Vec::new(),
            parts: Vec::new(),
        };
        fmt.emitter(&file!(), &line!());
        $crate::fmt!(
            @fmt
            $(
                $packet_type : $packet_value
                $(=> { $($deliminated)* })*,
            )*
        );
        fmt
    }}};
    (
        @$fmt:ident
        $(
            $packet_type:ident : $packet_value:expr
            $(=> { $($deliminated:tt)* })?,
        )*
    ) => {$(
        $fmt.types.push($crate::$packet_type);
        $fmt.parts.push(&$packet_value);
        $(
            $crate::fmt! {
                @$fmt
                $($deliminated)*
            }
            $fmt.types.push($crate::$packet_type.close_pair());
            $fmt.parts.push(&$packet_value);
        )*
    )*};
}

pub struct Format<'a> {
    #[doc(hidden)]
    pub types: Vec<Type>,
    #[doc(hidden)]
    pub parts: Vec<&'a dyn fmt::Display>,
}
impl<'a> fmt::Debug for Format<'a> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{{")?;
        for (t, p) in self.types.iter().zip(self.parts.iter()) {
            let p = format!("{}", p);
            write!(f, "{:?}:{:?} ", t, p)?;
        }
        write!(f, "}}")
    }
}
impl<'a> Format<'a> {
    pub fn emitter(&mut self, file: &'a &'static str, line: &'static u32) {
        self.types.push(FIELD_NAME);
        self.parts.push(&"meta:emitter_source_file");
        self.types.push(FIELD_VALUE);
        self.parts.push(file);

        self.types.push(FIELD_NAME);
        self.parts.push(&"meta:emitter_line");
        self.types.push(FIELD_VALUE);
        self.parts.push(line);
    }
    pub fn write(&self, out: &mut Atr) -> io::Result<()> {
        let mut buf = Vec::<u8>::new();
        use std::io::Write;
        for (&ty, msg) in self.types.iter().zip(self.parts.iter()) {
            let flags = ty.default_flags();
            if !out.status {
                if !flags.std_visible() {
                    continue;
                }
            }
            buf.clear();
            write!(buf, "{}", msg)?;
            let packet = Packet { flags, ty, msg: buf.as_slice() };
            out.write(&packet)?;
        }
        Ok(())
    }
}

pub fn stdin() -> Atr {
    unsafe {
        Atr::connect_raw(libc::STDIN_FILENO).unwrap()
    }
}

pub fn stdout() -> Atr {
    unsafe {
        Atr::connect_raw(libc::STDOUT_FILENO).unwrap()
    }
}

pub fn stderr() -> Atr {
    unsafe {
        Atr::connect_raw(libc::STDERR_FILENO).unwrap()
    }
}

pub struct ReadAtr {
    atr: Atr,
    in_buf: Vec<u8>,
    l: usize, // std_remaining = &in_buf[l..]
}
impl ReadAtr {
    pub fn read_packet<'a>(&'a mut self, buf: &'a mut Vec<u8>) -> io::Result<Packet<'a>> {
        let remaining = &self.in_buf[self.l..];
        if !remaining.is_empty() {
            self.l = self.in_buf.len();
            Ok(Packet {
                flags: Flags(F_STD_VISIBLE | F_SYNTHETIC /* I don't even know. I mean, "the kernel did this". Is this close enough? */),
                ty: LABEL,
                msg: remaining,
            })
        } else {
            self.atr.read(buf)
        }
    }
}
impl io::Read for ReadAtr {
    fn read(&mut self, out_buf: &mut [u8]) -> io::Result<usize> {
        if self.l >= self.in_buf.len() {
            self.l = 0;
            self.in_buf.clear();
        }
        while self.in_buf.is_empty() {
            let packet = self.atr.read(&mut self.in_buf)?;
            if !packet.flags.std_visible() {
                self.in_buf.clear();
            }
            self.l = 0;
        }
        let mut n = 0;
        for (i, o) in (&self.in_buf[self.l..]).iter().zip(out_buf.iter_mut()) {
            *o = *i;
            n += 1;
        }
        self.l += n;
        Ok(n)
    }
}

pub struct WriteAtr(Atr);
impl WriteAtr {
    pub fn write_packet(&mut self, packet: &Packet) -> io::Result<usize> {
        self.0.write(packet)
    }
}
impl io::Write for WriteAtr {
    fn write(&mut self, buf: &[u8]) -> io::Result<usize> {
        let p = Packet {
            flags: Flags(F_STD_VISIBLE | F_SYNTHETIC /* I don't even know. I mean, "the kernel did this". Is this close enough? */),
            ty: LABEL,
            msg: buf,
        };
        self.0.write(&p)
    }
    fn flush(&mut self) -> io::Result<()> {
        self.0.file.flush()
    }
}

#[cfg(unix)]
impl std::os::unix::io::AsRawFd for ReadAtr {
    fn as_raw_fd(&self) -> RawFd { self.atr.file.as_raw_fd() }
}
#[cfg(unix)]
impl std::os::unix::io::AsRawFd for WriteAtr {
    fn as_raw_fd(&self) -> RawFd { self.0.file.as_raw_fd() }
}
