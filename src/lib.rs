#![allow(non_snake_case)]

extern crate libc;

#[repr(C)]
struct vorbis_encoder_helper {
	private_data: *mut libc::c_void,
}

impl vorbis_encoder_helper {
	fn new() -> Self {
		unsafe {
			std::mem::zeroed()
		}
	}
}

pub struct Encoder {
	e: vorbis_encoder_helper,
}

impl Encoder {
	pub fn new(channels: u32, rate: u64, quality: f32) -> Result<Self, libc::c_int> {
		let mut enc = Encoder {
			e: vorbis_encoder_helper::new(),
		};
		let res = unsafe { vorbis_encoder_helper_init(
			&mut enc.e as *mut vorbis_encoder_helper,
			channels as libc::c_uint, rate as libc::c_ulong, quality as libc::c_float)};
		match res {
			0 => {
				Ok(enc)
			},
			_ => {
				Err(res)
			}
		}
	}

	pub fn encode(&mut self, samples: &Vec<i16>) -> Result<Vec<u8>, libc::c_int> {
		let res = unsafe { vorbis_encoder_helper_encode(
			&mut self.e as *mut vorbis_encoder_helper,
			samples.as_ptr() as *const libc::int16_t,
			samples.len() as libc::c_int)};
		if res != 0 {
			return Err(res);
		}
		let s = unsafe { vorbis_encoder_helper_get_data_length(
			&mut self.e as *mut vorbis_encoder_helper)};
		let mut v = vec![0u8; s as usize];
		unsafe { vorbis_encoder_helper_get_data(
			&mut self.e as *mut vorbis_encoder_helper,
			v[..].as_mut_ptr() as *mut libc::c_uchar);}
		Ok(v)
	}

	pub fn flush(&mut self) -> Result<Vec<u8>, libc::c_int> {
		let res = unsafe { vorbis_encoder_helper_flush(
			&mut self.e as *mut vorbis_encoder_helper)};
		if res != 0 {
			return Err(res);
		}
		let s = unsafe { vorbis_encoder_helper_get_data_length(
			&mut self.e as *mut vorbis_encoder_helper)};
		let mut v = vec![0u8; s as usize];
		unsafe { vorbis_encoder_helper_get_data(
			&mut self.e as *mut vorbis_encoder_helper,
			v[..].as_mut_ptr() as *mut libc::c_uchar);}
		Ok(v)
	}
}

impl Drop for Encoder {
	fn drop(&mut self) {
		unsafe {
			vorbis_encoder_helper_free(&mut self.e as *mut vorbis_encoder_helper);
		}
	}
}

extern "C" {
	fn vorbis_encoder_helper_init(
		hp: *mut vorbis_encoder_helper, ch: libc::c_uint, rt: libc::c_ulong,
		q:  libc::c_float) -> libc::c_int;
	fn vorbis_encoder_helper_encode(hp: *mut vorbis_encoder_helper, data: *const libc::int16_t,
		bits: libc::c_int) -> libc::c_int;
	fn vorbis_encoder_helper_flush(hp: *mut vorbis_encoder_helper) -> libc::c_int;
	fn vorbis_encoder_helper_get_data_length(hp: *const vorbis_encoder_helper) -> libc::c_uint;
	fn vorbis_encoder_helper_get_data(hp: *mut vorbis_encoder_helper, data: *mut libc::c_uchar);
	fn vorbis_encoder_helper_free(hp: *mut vorbis_encoder_helper);
}
