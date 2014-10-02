using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Xml.Linq;
using Microsoft.Office.Interop.OneNote;

namespace OneNote
{
    class Program
    {
        static Application onenoteApp = new Application();
        static XNamespace namesp = null;

       public static void Main(string[] args)
        {
            GetNamespace();
            string notebookId = GetObjectId(null, HierarchyScope.hsNotebooks, "Notes");
            string sectionId = GetObjectId(notebookId, HierarchyScope.hsSections, "Tasks");
            string pageId = CreatePage(sectionId, "Shopping List");
        }

        static void GetNamespace()
        {
            string xml;
            onenoteApp.GetHierarchy(null, HierarchyScope.hsNotebooks, out xml);

            var doc = XDocument.Parse(xml);
            namesp = doc.Root.Name.Namespace;
        }

        static string GetObjectId(string parentId, HierarchyScope scope, string objectName)
        {
            string xml;
            onenoteApp.GetHierarchy(parentId, scope, out xml);

            var doc = XDocument.Parse(xml);
            var nodeName = "";

            switch (scope)
            {
                case (HierarchyScope.hsNotebooks): nodeName = "Notebook"; break;
                case (HierarchyScope.hsPages): nodeName = "Page"; break;
                case (HierarchyScope.hsSections): nodeName = "Section"; break;
                default:
                    return null;
            }

            var node = doc.Descendants(namesp + nodeName).Where(n => n.Attribute("name").Value == objectName).FirstOrDefault();
            var ID = node.Attribute("ID").Value;
            return ID;
        }

        static string CreatePage(string sectionId, string pageName)
        {
            // Create the new page
            string pageId;
            string pageText = "Buy Milk";
            onenoteApp.CreateNewPage(sectionId, out pageId, NewPageStyle.npsBlankPageWithTitle);

            // Get the title and set it to page name
            string xml;
            onenoteApp.GetPageContent(pageId, out xml, PageInfo.piAll);
            var doc = XDocument.Parse(xml);

            var title = doc.Descendants(namesp + "T").First();
            title.Value = pageName;

            // Update the page
            onenoteApp.UpdatePageContent(doc.ToString(), DateTime.MinValue);

            if (doc != null)
            {
                var page = new XDocument(new XElement(namesp + "Page",
                                           new XElement(namesp + "Outline",
                                             new XElement(namesp + "OEChildren",
                                               new XElement(namesp + "OE",
                                                 new XElement(namesp + "T",
                                                   new XCData(pageText)))))));

                page.Root.SetAttributeValue("ID", pageId);
                onenoteApp.UpdatePageContent(page.ToString(), DateTime.MinValue);
            }

            return pageId;
        }

    }
}

