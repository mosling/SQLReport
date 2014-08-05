::MAIN
select playlist.playlistid, playlist.name as plname, track.name as tname 
from playlist, playlisttrack, track where playlisttrack.playlistid = playlist.playlistid and playlisttrack.trackid = track.trackid
order by playlist.name
