::ARTIST
select * from artist order by name

::ALBUM
::# all album entries for this artist
select * from album where ArtistId = ${ArtistId}