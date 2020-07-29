import httpclient, lexbor
export httpclient, lexbor

proc `{}`*(Self: type[HttpClient]): Self =
   result = new_HttpClient()
