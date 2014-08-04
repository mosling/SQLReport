::MAIN
select * from artist order by name

::ALBUM
select * from album where ArtistId = ${ArtistId}