﻿using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace BonExtension.Generators
{
    public class Folder : Object
    {
        new public class Template
        {
            public static readonly string Containment =
@"
        public IEnumerable<{0}> Contained{0}s
        {{
            get
            {{
                MgaObjects children = mgaObject.ChildObjects;
                foreach (MgaObject o in children)
                {{
                    if (o.MetaBase.Name == ""{0}"")
                        yield return new {0}(o as {1});
                }}
            }}
        }}
";
            public static readonly string Class =
@"
namespace {0}
{{
    public class {1} : {2}
    {{
        internal {3} mgaObject;
        internal static {6} mgaMetaObject = {0}.Util.GetMetaByName(""{7}"") as {6};
        public {1}({3} mgaObject)
        {{
            this.mgaObject = mgaObject;
        }}

{8}
    
        #region Common
{4}
        #endregion

        #region Containment
{5}
        #endregion
    }}
}}
";        
        }

        public Folder(MGALib.IMgaAtom mgaObject)
        {            
            baseInterfaceName = "IFolder";

            memberType = "IMgaFolder";
            namespaceName = General.NamespaceName;

            if (mgaObject != null)
            {
                className = mgaObject.Name;

                //rootfolder is initialized with mgaObject == null
                this.mgaObject = mgaObject;

                if (Object.ElementsByName.ContainsKey(mgaObject.Name))
                {
                    //TODO
                    //throw new Exception(string.Format("Element name is not unique: {0}", mgaObject.Name));
                    GME.CSharp.BonExtender.Errors.Add(string.Format("Element name is not unique: {0}", mgaObject.Name));
                }
                else
                {
                    Object.ElementsByName.Add(mgaObject.Name, this);
                }

                //if (Object.ElementsByName.ContainsKey(mgaObject.Name))
                //{
                //    throw new Exception(string.Format("Element name is not unique: {0}", mgaObject.Name));
                //}

                //Object.ElementsByName.Add(mgaObject.Name, this);

                if (InRootFolder)
                    RootFolder.RootObject.Add(this);
            }
        }

        #region Containment
        protected virtual IEnumerable<Object> Contained
        {
            get
            {
                foreach (MGALib.IMgaObject mgaObject in this.MgaObjects)
                {
                    MGALib.IMgaFCO fco = mgaObject as MGALib.IMgaFCO;
                    foreach (MGALib.IMgaConnPoint conn in fco.PartOfConns)
                    {
                        if (conn.Owner.Meta.Name == "FolderContainment" && conn.ConnRole == "dst")
                        {
                            foreach (MGALib.IMgaConnPoint connOther in conn.Owner.ConnPoints)
                            {
                                if (connOther.ConnRole == "src")
                                {
                                    //connOther.target: contained
                                    if (connOther.target.MetaBase.Name.Contains("Proxy"))
                                    {
                                        if (Object.ProxyCache.ContainsKey(connOther.target.Name))
                                            yield return Object.ElementsByName[Object.ProxyCache[connOther.target.Name]] as FCO;
                                        else
                                            GME.CSharp.BonExtender.Errors.Add("Proxy '" + connOther.target.Name + "' is not found");
                                    }
                                    else
                                    {
                                        if (Object.ElementsByName.ContainsKey(connOther.target.Name))
                                            yield return Object.ElementsByName[connOther.target.Name] as Object;
                                        else
                                        {
                                            //todo
                                            GME.CSharp.BonExtender.Errors.Add(connOther.target.Name + " is not found");
                                            //throw new Exception(connOther.target.Name + " is not cached");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        public string GenerateContainments()
        {
            StringBuilder sb = new StringBuilder();

            sb.Append(generateOwnContainments());

            return sb.ToString();
        }
        private string generateOwnContainments()
        {
            StringBuilder sb = new StringBuilder();
            foreach (Object cont in Contained)
            {
                if (cont is Folder)
                    sb.Append(generateContainment(cont.className, cont.memberType));
                else
                    sb.Append(generateContainment(cont as FCO));
            }
            return sb.ToString();
        }
        private string generateContainment(string typename, string type)
        {
            //addCheckConnectionName(typename);

            StringBuilder sb = new StringBuilder();
            sb.AppendFormat(Folder.Template.Containment, typename, type);

            return sb.ToString();
        }
        private string generateContainment(FCO current)
        {
            StringBuilder inner = new StringBuilder();

            inner.AppendFormat(Model.Template.ContainmentInner, current.className, current.ProperClassName, current.memberType);
            if (current.HasChildren)
            {
                //and add all of the children
                foreach (FCO child in current.ChildrenRecursive)
                {
                    inner.AppendFormat(Model.Template.ContainmentInner, child.className, child.ProperClassName, child.memberType);
                }
            }

            StringBuilder sb = new StringBuilder();
            sb.AppendFormat(Model.Template.Containment, current.className, inner.ToString());

            return sb.ToString();
        }
        #endregion

        public override string GenerateClass()
        {
            List<string> crnews = new List<string>();

            StringBuilder sb = new StringBuilder();
            sb.Append(Init());

            sb.AppendFormat(
                Folder.Template.Class,
                namespaceName,
                className,
                baseInterfaceName,
                memberType,
                GenerateCommon(),
                GenerateContainments(),
                "IMgaMeta" + memberType.Substring(4),
                className,
                GenerateCreateNews(ref crnews, this));

            return sb.ToString();
        }

        public override void Save()
        {
            Directory.CreateDirectory(Path.Combine(General.Path, General.ClassName));
            Directory.CreateDirectory(Path.Combine(General.Path, General.ClassName) + "\\BonExtension");
            using (TextWriter tw = new StreamWriter(Path.Combine(General.Path, General.ClassName) + @"\BonExtension\" + this.Name + ".cs"))
            {
                tw.WriteLine(GenerateClass());
            }
        }
    }
}
