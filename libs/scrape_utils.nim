import httpclient, pkg/htmlparser, pkg/htmlparser/xmltree
export httpclient, htmlparser, xmltree

type Html* = XmlNode

proc `{}`*(Self: type[HttpClient]): Self =
   result = new_HttpClient()

proc find_first_class*(self: Html, class_name: string): Html =
   proc rec(self: Html, class_name: string): Html =
      if self.kind == xn_element and self.attr("class") == class_name:
         return self
      if self.kind == xn_element:
         for child in self:
            result = rec(child, class_name)
            if result != nil:
               return
   result = rec(self, class_name)
   if result == nil:
      raise new_Exception(ValueError, "failed to find class with name: " & class_name)

proc find_first*(self: Html, tag: HtmlTag): Html =
   proc rec(self: Html, tag: HtmlTag): Html =
      if self.kind == xn_element and self.html_tag == tag:
         return self
      if self.kind == xn_element:
         for child in self:
            result = rec(child, tag)
            if result != nil:
               return
   result = rec(self, tag)
   if result == nil:
      raise new_Exception(ValueError, "failed to find tag: " & $tag)
